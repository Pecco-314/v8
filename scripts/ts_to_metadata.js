#!/usr/bin/env node
// Entry: node scripts/ts_to_metadata.js <path/to/test.ts> [--outDir <dir>]
// 1) 使用 tsc 将 TS 编译为 JS（默认输出到 TS 所在目录）
// 2) 基于 TS 类型 + d8 AST 提取的字节码偏移生成 metadata 文件
//    不依赖手写或外部模板，确保由 TS 驱动

const ts = require('typescript');
const crypto = require('crypto');
const { execFileSync } = require('child_process');
const { existsSync, statSync, mkdirSync, writeFileSync, readFileSync, mkdtempSync, rmSync } = require('fs');
const path = require('path');

function usage() {
	console.error('用法: node scripts/ts_to_metadata.js <ts_file> [--outDir <dir>]');
	process.exit(1);
}

function main() {
	const args = process.argv.slice(2);
	if (args.length < 1) usage();

	let tsFile = args[0];
	let outDir = null;
	for (let i = 1; i < args.length; i++) {
		if (args[i] === '--outDir' && i + 1 < args.length) {
			outDir = args[i + 1];
			i++;
		}
	}

	if (!existsSync(tsFile)) {
		console.error(`❌ TS 文件不存在: ${tsFile}`);
		process.exit(1);
	}
	if (statSync(tsFile).isDirectory()) {
		console.error('❌ 需要提供文件路径，而不是目录');
		process.exit(1);
	}

	const absTs = path.resolve(tsFile);
	const tsDir = path.dirname(absTs);
	const base = path.basename(tsFile, '.ts');
	const targetDir = outDir ? path.resolve(outDir) : tsDir;
	const jsFile = path.join(targetDir, `${base}.js`);

	console.log('=== Step 1: tsc 编译 TS -> JS ===');
	execFileSync('npx', [
		'tsc',
		absTs,
		'--target',
		'es2020',
		'--module',
		'none',
		'--noEmitOnError',
		'--outDir',
		targetDir,
	], { stdio: 'inherit' });

	if (!existsSync(jsFile)) {
		console.error(`❌ 未找到生成的 JS: ${jsFile}`);
		process.exit(1);
	}

	console.log('=== Step 2: 解析 TS 类型 ===');
	const typeInfo = collectTypesFromTs(absTs);

	console.log('=== Step 3: 解析 JS 偏移 (d8 --print-ast) ===');
	const offsets = collectOffsetsFromJs(jsFile, typeInfo);

	console.log('=== Step 4: 写出 metadata ===');
	const metadataPath = writeMetadata(jsFile, typeInfo, offsets);
	console.log(`✅ metadata 已写入: ${metadataPath}`);

	console.log('\n✅ ts_to_metadata 完成');
}

try {
	main();
} catch (error) {
	console.error(`❌ ${error.message}`);
	process.exit(1);
}

// ---- helpers ----

function collectTypesFromTs(tsFile) {
	const src = readFileSync(tsFile, 'utf8');
	const sf = ts.createSourceFile(tsFile, src, ts.ScriptTarget.ES2020, true);
	const namedTypeNodes = collectNamedTypeNodes(sf);
	const ctx = { namedTypeNodes };
	const results = [];
	const nameCounts = new Map();

	function appendFunction(name, funcLikeNode) {
		const params = funcLikeNode.parameters.map((p) => mapTsType(p.type, ctx));
		const ret = mapTsType(funcLikeNode.type, ctx);
		const occurrence = nameCounts.get(name) || 0;
		nameCounts.set(name, occurrence + 1);
		results.push({ name, occurrence, params, ret });
	}

	function visit(node) {
		if (ts.isFunctionDeclaration(node) && node.name && node.body) {
			appendFunction(node.name.text, node);
		}

		if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.initializer) {
			const init = node.initializer;
			if (ts.isFunctionExpression(init) || ts.isArrowFunction(init)) {
				appendFunction(node.name.text, init);
			}
		}

		ts.forEachChild(node, visit);
	}

	visit(sf);

	if (results.length === 0) {
		console.warn('⚠️ 未找到函数声明');
	}
	return results;
}

function collectNamedTypeNodes(sf) {
	const namedTypeNodes = new Map();
	sf.forEachChild((node) => {
		if ((ts.isInterfaceDeclaration(node) || ts.isClassDeclaration(node)) && node.name) {
			namedTypeNodes.set(node.name.text, node.members);
		}
	});
	return namedTypeNodes;
}

function mapObjectMembers(members, ctx, stack) {
	const parts = [];
	for (const member of members) {
		const isProp = ts.isPropertySignature(member) || ts.isPropertyDeclaration(member);
		if (isProp && member.name && member.type) {
			const key = member.name.getText();
			const val = mapTsType(member.type, ctx, stack);
			parts.push(`${key}:${val}`);
		}
	}
	return parts;
}

function mapTsType(typeNode, ctx = {}, stack = new Set()) {
	if (!typeNode) return 'any';
	switch (typeNode.kind) {
		case ts.SyntaxKind.StringKeyword:
			return 'str';
		case ts.SyntaxKind.NumberKeyword:
			return 'num';
		case ts.SyntaxKind.BooleanKeyword:
			return 'bool';
		case ts.SyntaxKind.SymbolKeyword:
			return 'symbol';
		case ts.SyntaxKind.BigIntKeyword:
			return 'bigint';
		case ts.SyntaxKind.VoidKeyword:
			return 'void';
		case ts.SyntaxKind.AnyKeyword:
			return 'any';
		case ts.SyntaxKind.ArrayType: {
			const elem = mapTsType(typeNode.elementType, ctx, stack);
			return `arr<${elem}>`;
		}
		case ts.SyntaxKind.TupleType: {
			const elems = typeNode.elements.map((e) => mapTsType(e, ctx, stack));
			return `tuple<${elems.join(',')}>`;
		}
		case ts.SyntaxKind.TypeReference: {
			const name = typeNode.typeName.getText();
			const lower = name.toLowerCase();
			if (lower === 'rawint32') return 'rawint32';
			if (lower === 'rawuint32') return 'rawuint32';
			if (lower === 'rawint64') return 'rawint64';
			if (lower === 'rawuint64') return 'rawuint64';
			const members = ctx.namedTypeNodes?.get(name);
			if (members) {
				if (stack.has(name)) return 'any'; // 防止递归引用死循环
				stack.add(name);
				const parts = mapObjectMembers(members, ctx, stack);
				stack.delete(name);
				if (parts.length > 0) return `obj{${parts.join(',')}}`;
			}
			return 'any';
		}
		case ts.SyntaxKind.TypeLiteral: {
			// 对象字面量: obj{field:type,...}
			const parts = mapObjectMembers(typeNode.members, ctx, stack);
			if (parts.length > 0) return `obj{${parts.join(',')}}`;
			return 'any';
		}
		default:
			return 'any';
	}
}

function collectOffsetsFromJs(jsFile, typeInfo) {
	const d8Path = process.env.D8_PATH || path.join('out.gn', 'x64.debug', 'd8');
	if (!existsSync(d8Path)) {
		console.error(`❌ 未找到 d8: ${d8Path}`);
		process.exit(1);
	}
	// Minimal assertion stubs to avoid loading full mjsunit (which produces huge AST output).
	const stubPrelude = [
		'function assertEquals(){}',
		'function assertOptimized(){}',
		'function assertUnoptimized(){}',
		'function assertTrue(){}',
	].join(';');
	const workspaceTmpDir = path.join(process.cwd(), 'tmp');
	if (!existsSync(workspaceTmpDir)) mkdirSync(workspaceTmpDir, { recursive: true });
	const tmpDir = mkdtempSync(path.join(workspaceTmpDir, 'tsmeta-'));
	const harness = path.join(tmpDir, 'harness.js');

	try {
		const jsFileLiteral = JSON.stringify(jsFile);
		writeFileSync(harness, `${stubPrelude}; load(${jsFileLiteral});`);

		const output = execFileSync(
			d8Path,
			['--allow-natives-syntax', '--print-ast', harness],
			{ encoding: 'utf8', maxBuffer: 50 * 1024 * 1024 }
		);

		const lines = output.split(/\r?\n/);
		const nameToPositions = new Map();
		let pendingPos = null;
		let pendingName = null;

		function flushPending() {
			if (pendingPos === null || pendingName === null) return;
			if (!nameToPositions.has(pendingName)) nameToPositions.set(pendingName, []);
			nameToPositions.get(pendingName).push(pendingPos);
		}

		for (const line of lines) {
			const funcMatch = line.match(/FUNC(?: LITERAL)? at (\d+)/);
			if (funcMatch) {
				flushPending();
				pendingPos = Number(funcMatch[1]);
				pendingName = null;
				continue;
			}

			if (pendingPos !== null && pendingName === null) {
				const nameMatch = line.match(/NAME "([^"]+)"/);
				if (nameMatch) {
					pendingName = nameMatch[1];
				}
			}
		}
		flushPending();

		const offsets = new Map();
		const missing = [];
		for (const info of typeInfo) {
			const positions = nameToPositions.get(info.name) || [];
			const pos = positions[info.occurrence];
			const key = makeFunctionKey(info.name, info.occurrence);
			if (pos === undefined) {
				missing.push(`${info.name}#${info.occurrence}`);
				continue;
			}
			offsets.set(key, pos);
		}

		if (missing.length > 0) {
			throw new Error(
				`AST 偏移匹配失败，缺失 ${missing.length} 个函数: ${missing.join(', ')}`
			);
		}

		return offsets;
	} finally {
		rmSync(tmpDir, { recursive: true, force: true });
	}
}

function makeFunctionKey(name, occurrence) {
	return `${name}#${occurrence}`;
}

function writeMetadata(jsFile, typeInfo, offsets) {
	const jsRel = path.relative(process.cwd(), jsFile);
	const jsContent = readFileSync(jsFile);
	const hash = crypto.createHash('sha256').update(jsContent).digest('hex');

	const lines = [];
	lines.push(`# Auto-generated by ts_to_metadata.js`);
	lines.push(`# Source: ${jsRel}`);
	lines.push(`# JS SHA256: ${hash}`);
	lines.push('');

	typeInfo.forEach(({ name, occurrence, params, ret }) => {
		const key = makeFunctionKey(name, occurrence);
		const pos = offsets.get(key);
		if (pos === undefined) return; // skip if missing
		const paramsStr = ['any', ...params].join(' ');
		lines.push(`${pos} @params ${paramsStr} @ret ${ret}  # ${name}`);
	});

	const outDir = path.join(process.cwd(), 'test/mjsunit/compiler/type-injector/metadata');
	if (!existsSync(outDir)) mkdirSync(outDir, { recursive: true });
	const outPath = path.join(outDir, `${hash}.metadata`);
	writeFileSync(outPath, lines.join('\n'));
	return outPath;
}
