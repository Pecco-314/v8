#!/usr/bin/env node
// Entry: node scripts/ts_to_metadata.js <path/to/test.ts> [--outDir <dir>]
// 1) 使用 tsc 将 TS 编译为 JS（默认输出到 TS 所在目录）
// 2) 基于 TS 类型 + Source Map + JS AST 生成 metadata 文件
//    通过 source map 做 TS<->JS 位置映射，避免 name+occurrence 脆弱绑定

const ts = require('typescript');
const crypto = require('crypto');
const { execFileSync } = require('child_process');
const { existsSync, statSync, mkdirSync, writeFileSync, readFileSync } = require('fs');
const path = require('path');

const BASE64_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
const BASE64_MAP = Object.fromEntries([...BASE64_CHARS].map((c, i) => [c, i]));

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
	const tscVersion = getTscVersion();

	console.log('=== Step 1: tsc 编译 TS -> JS ===');
	execFileSync('npx', [
		'tsc',
		absTs,
		'--target',
		'es2020',
		'--module',
		'none',
		'--noEmitOnError',
		'--sourceMap',
		'--outDir',
		targetDir,
	], { stdio: 'inherit' });

	if (!existsSync(jsFile)) {
		console.error(`❌ 未找到生成的 JS: ${jsFile}`);
		process.exit(1);
	}
	const mapFile = `${jsFile}.map`;
	if (!existsSync(mapFile)) {
		console.error(`❌ 未找到生成的 Source Map: ${mapFile}`);
		process.exit(1);
	}

	console.log('=== Step 2: 解析 TS 类型 ===');
	const typeInfo = collectTypesFromTs(absTs);

	console.log('=== Step 3: 通过 Source Map 对齐 TS/JS 源码位置 ===');
	const offsets = collectOffsetsFromJs(jsFile, mapFile, absTs, typeInfo);

	console.log('=== Step 4: 写出 metadata ===');
	const metadataPath = writeMetadata(absTs, jsFile, typeInfo, offsets, tscVersion);
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

function getTscVersion() {
	try {
		const output = execFileSync('npx', ['tsc', '--version'], {
			encoding: 'utf8',
			stdio: ['ignore', 'pipe', 'pipe'],
		});
		return output.trim();
	} catch {
		return 'unknown';
	}
}

function collectTypesFromTs(tsFile) {
	const src = readFileSync(tsFile, 'utf8');
	const sf = ts.createSourceFile(tsFile, src, ts.ScriptTarget.ES2020, true);
	const namedTypeNodes = collectNamedTypeNodes(sf);
	const ctx = { namedTypeNodes };
	const results = [];

	function appendFunction(name, funcLikeNode) {
		const params = funcLikeNode.parameters.map((p) => mapTsType(p.type, ctx));
		const ret = mapTsType(funcLikeNode.type, ctx);
		results.push({
			id: results.length,
			name,
			params,
			ret,
			start: funcLikeNode.getStart(sf),
			end: funcLikeNode.end,
		});
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

function collectOffsetsFromJs(jsFile, mapFile, tsFile, typeInfo) {
	const src = readFileSync(jsFile, 'utf8');
	const sf = ts.createSourceFile(jsFile, src, ts.ScriptTarget.ES2020, true, ts.ScriptKind.JS);
	const jsLineStarts = computeLineStarts(src);

	const tsSrc = readFileSync(tsFile, 'utf8');
	const tsLineStarts = computeLineStarts(tsSrc);

	const rawMap = JSON.parse(readFileSync(mapFile, 'utf8'));
	const mapIndex = buildSourceMapIndex(rawMap);
	const offsets = new Map();

	function appendFunction(node) {
		const jsPos = computeFunctionSourcePosition(node, sf, src);
		const generated = offsetToLineCol(jsLineStarts, jsPos);
		const original = originalPositionFor(mapIndex, generated.line, generated.column);
		if (!original) return;

		const tsPos = lineColToOffset(tsLineStarts, original.line, original.column);
		const best = findBestTypeInfoByOffset(typeInfo, tsPos);
		if (!best) return;

		if (!offsets.has(best.id)) {
			offsets.set(best.id, jsPos);
		}
	}

	function visit(node) {
		if (ts.isFunctionDeclaration(node) && node.name && node.body) {
			appendFunction(node);
		}

		if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.initializer) {
			const init = node.initializer;
			if (ts.isFunctionExpression(init) || ts.isArrowFunction(init)) {
				appendFunction(init);
			}
		}

		ts.forEachChild(node, visit);
	}

	visit(sf);

	const missing = [];
	for (const info of typeInfo) {
		if (!offsets.has(info.id)) {
			missing.push(info.name);
		}
	}

	if (missing.length > 0) {
		throw new Error(`源码位置匹配失败，缺失 ${missing.length} 个函数: ${missing.join(', ')}`);
	}

	return offsets;
}

function computeFunctionSourcePosition(node, sf, src) {
	const start = node.getStart(sf);
	const bodyPos = node.body ? node.body.pos : node.end;
	const paren = src.indexOf('(', start);

	if (paren !== -1 && paren < bodyPos) {
		return paren;
	}

	if (ts.isArrowFunction(node) && node.parameters.length > 0) {
		return node.parameters[0].getStart(sf);
	}

	throw new Error(
		`无法稳定计算函数源码位置（kind=${ts.SyntaxKind[node.kind]} start=${start}）`
	);
}

function findBestTypeInfoByOffset(typeInfo, offset) {
	let best = null;
	let bestSpan = Number.POSITIVE_INFINITY;
	for (const info of typeInfo) {
		if (info.start <= offset && offset < info.end) {
			const span = info.end - info.start;
			if (span < bestSpan) {
				best = info;
				bestSpan = span;
			}
		}
	}
	return best;
}

function computeLineStarts(text) {
	const starts = [0];
	for (let i = 0; i < text.length; i++) {
		if (text[i] === '\n') starts.push(i + 1);
	}
	return starts;
}

function offsetToLineCol(lineStarts, offset) {
	let low = 0;
	let high = lineStarts.length - 1;
	while (low <= high) {
		const mid = (low + high) >> 1;
		if (lineStarts[mid] <= offset) {
			if (mid === lineStarts.length - 1 || lineStarts[mid + 1] > offset) {
				return { line: mid + 1, column: offset - lineStarts[mid] };
			}
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	return { line: 1, column: offset };
}

function lineColToOffset(lineStarts, line, column) {
	const lineStart = lineStarts[Math.max(0, line - 1)] || 0;
	return lineStart + column;
}

function buildSourceMapIndex(rawMap) {
	const lines = [];
	const mappingLines = (rawMap.mappings || '').split(';');
	let previousSource = 0;
	let previousOriginalLine = 0;
	let previousOriginalColumn = 0;

	for (const lineText of mappingLines) {
		const segments = [];
		let generatedColumn = 0;
		if (lineText.length > 0) {
			for (const rawSegment of lineText.split(',')) {
				if (!rawSegment) continue;
				const decoded = decodeVlqSegment(rawSegment);
				generatedColumn += decoded[0] || 0;
				if (decoded.length >= 4) {
					previousSource += decoded[1];
					previousOriginalLine += decoded[2];
					previousOriginalColumn += decoded[3];
					segments.push({
						generatedColumn,
						source: previousSource,
						originalLine: previousOriginalLine + 1,
						originalColumn: previousOriginalColumn,
					});
				}
			}
		}
		lines.push(segments);
	}

	return {
		lines,
		sources: rawMap.sources || [],
		sourceRoot: rawMap.sourceRoot || '',
	};
}

function originalPositionFor(index, generatedLine, generatedColumn) {
	const lineSegments = index.lines[generatedLine - 1];
	if (!lineSegments || lineSegments.length === 0) return null;

	let best = null;
	for (const segment of lineSegments) {
		if (segment.generatedColumn <= generatedColumn) {
			best = segment;
		} else {
			break;
		}
	}

	if (!best) return null;
	return {
		source: index.sources[best.source] || null,
		line: best.originalLine,
		column: best.originalColumn,
	};
}

function decodeVlqSegment(segment) {
	const values = [];
	let value = 0;
	let shift = 0;

	for (let i = 0; i < segment.length; i++) {
		const digit = BASE64_MAP[segment[i]];
		if (digit === undefined) break;

		const continuation = (digit & 32) !== 0;
		const chunk = digit & 31;
		value += chunk << shift;

		if (continuation) {
			shift += 5;
			continue;
		}

		const isNegative = (value & 1) === 1;
		let decoded = value >> 1;
		if (isNegative) decoded = -decoded;
		values.push(decoded);

		value = 0;
		shift = 0;
	}

	return values;
}

function sha256HexFromText(text) {
	return crypto.createHash('sha256').update(text, 'utf8').digest('hex');
}

function writeMetadata(tsFile, jsFile, typeInfo, offsets, tscVersion) {
	const generatorName = 'ts_to_metadata.js';
	const generatorPath = path.join(process.cwd(), 'scripts', generatorName);
	const toolContent = readFileSync(generatorPath);
	const toolHash = crypto.createHash('sha256').update(toolContent).digest('hex');
	const tsContent = readFileSync(tsFile);
	const tsHash = crypto.createHash('sha256').update(tsContent).digest('hex');
	const tsRel = path.relative(process.cwd(), tsFile);
	const jsRel = path.relative(process.cwd(), jsFile);
	const jsContent = readFileSync(jsFile);
	const hash = crypto.createHash('sha256').update(jsContent).digest('hex');
	const generatedAt = new Date().toISOString();

	const lines = [];
	lines.push(`# Auto-generated by ts_to_metadata.js`);
	lines.push(`# Metadata-Version: 2`);
	lines.push(`# Provenance-Generator: ${generatorName}`);
	lines.push(`# Provenance-Generator-SHA256: ${toolHash}`);
	lines.push(`# Provenance-TS-Source: ${tsRel}`);
	lines.push(`# Provenance-TS-SHA256: ${tsHash}`);
	lines.push(`# Provenance-TSC-Version: ${tscVersion}`);
	lines.push(`# Provenance-Generated-At: ${generatedAt}`);
	lines.push(`# Source: ${jsRel}`);
	lines.push(`# JS SHA256: ${hash}`);

	const entryLines = [];

	typeInfo.forEach(({ id, name, params, ret }) => {
		const pos = offsets.get(id);
		if (pos === undefined) return;
		const paramsStr = ['any', ...params].join(' ');
		entryLines.push(`${pos} @params ${paramsStr} @ret ${ret}  # ${name}`);
	});
	const entriesSha = sha256HexFromText(entryLines.join('\n'));
	const provenanceStampInput = [
		'v1',
		generatorName,
		toolHash,
		tsRel,
		tsHash,
		jsRel,
		hash,
		tscVersion,
		entriesSha,
	].join('\n');
	const provenanceStamp = sha256HexFromText(provenanceStampInput);
	lines.push(`# Entries SHA256: ${entriesSha}`);
	lines.push(`# Provenance-Stamp: ${provenanceStamp}`);
	lines.push('');
	lines.push(...entryLines);

	const outDir = path.join(process.cwd(), 'test/mjsunit/compiler/type-injector/metadata');
	if (!existsSync(outDir)) mkdirSync(outDir, { recursive: true });
	const outPath = path.join(outDir, `${hash}.metadata`);
	writeFileSync(outPath, lines.join('\n'));
	return outPath;
}
