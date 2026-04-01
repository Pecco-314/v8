#!/usr/bin/env node

const ts = require('typescript');
const path = require('path');
const { existsSync, mkdirSync, readFileSync, writeFileSync } = require('fs');
const { execFileSync } = require('child_process');

function usage() {
	console.error(
		'用法: node scripts/ts_locals_to_wrappers.js <ts_file> [--out <out.ts>] [--run-metadata] [--metadataOutDir <dir>]'
	);
	process.exit(1);
}

function parseArgs(argv) {
	if (argv.length < 1) usage();
	const result = {
		tsFile: argv[0],
		outFile: null,
		runMetadata: false,
		metadataOutDir: null,
	};

	for (let i = 1; i < argv.length; i++) {
		const arg = argv[i];
		if (arg === '--out' && i + 1 < argv.length) {
			result.outFile = argv[++i];
		} else if (arg === '--run-metadata') {
			result.runMetadata = true;
		} else if (arg === '--metadataOutDir' && i + 1 < argv.length) {
			result.metadataOutDir = argv[++i];
		} else {
			console.error(`未知参数: ${arg}`);
			usage();
		}
	}

	return result;
}

function resolveDefaultOutFile(absInput) {
	const repoTmpDir = path.join(process.cwd(), 'tmp', 'preprocessed');
	if (!existsSync(repoTmpDir)) mkdirSync(repoTmpDir, { recursive: true });
	const base = path.basename(absInput, '.ts');
	return path.join(repoTmpDir, `${base}.locals.ts`);
}

function isFunctionLikeContainer(node) {
	return (
		ts.isFunctionDeclaration(node) ||
		ts.isFunctionExpression(node) ||
		ts.isArrowFunction(node) ||
		ts.isMethodDeclaration(node) ||
		ts.isConstructorDeclaration(node) ||
		ts.isGetAccessorDeclaration(node) ||
		ts.isSetAccessorDeclaration(node)
	);
}

function parseTypeNodeFromText(typeText) {
	const sf = ts.createSourceFile(
		'__type_parse__.ts',
		`type __T = ${typeText};`,
		ts.ScriptTarget.ES2020,
		true,
		ts.ScriptKind.TS
	);
	const alias = sf.statements.find(ts.isTypeAliasDeclaration);
	return alias ? alias.type : null;
}

function shouldSkipTypeText(typeText) {
	if (!typeText) return true;
	const normalized = typeText.trim();
	if (!normalized) return true;
	if (normalized === 'any' || normalized === 'unknown' || normalized === 'never') return true;
	return false;
}

function inferTypeText(decl, checker) {
	if (decl.type) {
		return decl.type.getText();
	}
	if (!decl.initializer || !checker) return null;

	try {
		let inferred = checker.getTypeAtLocation(decl.initializer);
		inferred = checker.getBaseTypeOfLiteralType(inferred);
		const typeText = checker.typeToString(
			inferred,
			decl.initializer,
			ts.TypeFormatFlags.NoTruncation |
				ts.TypeFormatFlags.UseAliasDefinedOutsideCurrentScope |
				ts.TypeFormatFlags.InTypeAlias
		);
		return typeText;
	} catch (_) {
		return null;
	}
}

function transformFile(sourceFile, checker) {
	let wrapperCounter = 0;
	let transformedCount = 0;
	const transformedLocals = [];
	const fileLevelHelpers = [];

	const transformer = (context) => {
		const factory = context.factory;

		function visit(node, inFunctionBody) {
			const childInFunctionBody = inFunctionBody || isFunctionLikeContainer(node);

			if (ts.isBlock(node) && inFunctionBody) {
				const newStatements = [];
				for (const statement of node.statements) {
					if (!ts.isVariableStatement(statement)) {
						newStatements.push(visit(statement, true));
						continue;
					}

					let changed = false;
					const newDecls = statement.declarationList.declarations.map((decl) => {
						if (!ts.isIdentifier(decl.name) || !decl.initializer) {
							return decl;
						}
						if (
							ts.isCallExpression(decl.initializer) &&
							ts.isIdentifier(decl.initializer.expression) &&
							decl.initializer.expression.text.startsWith('__ti_local_wrap_')
						) {
							return decl;
						}

						const typeText = inferTypeText(decl, checker);
						if (shouldSkipTypeText(typeText)) {
							return decl;
						}

						const paramTypeNode = parseTypeNodeFromText(typeText);
						const returnTypeNode = parseTypeNodeFromText(typeText);
						if (!paramTypeNode || !returnTypeNode) {
							return decl;
						}

						changed = true;
						transformedCount += 1;
						const wrapperName = `__ti_local_wrap_${++wrapperCounter}`;
						transformedLocals.push({
							name: decl.name.text,
							type: typeText,
							wrapper: wrapperName,
						});

						fileLevelHelpers.push(
							factory.createFunctionDeclaration(
								undefined,
								undefined,
								factory.createIdentifier(wrapperName),
								undefined,
								[
									factory.createParameterDeclaration(
										undefined,
										undefined,
										factory.createIdentifier('__value'),
										undefined,
										paramTypeNode,
										undefined
									),
								],
								returnTypeNode,
								factory.createBlock(
									[
										factory.createTryStatement(
											factory.createBlock(
												[factory.createReturnStatement(factory.createIdentifier('__value'))],
												true
											),
											undefined,
											factory.createBlock([], true)
										),
									],
									true
								)
							)
						);

						const wrappedInit = factory.createCallExpression(
							factory.createIdentifier(wrapperName),
							undefined,
							[decl.initializer]
						);
						return factory.updateVariableDeclaration(
							decl,
							decl.name,
							decl.exclamationToken,
							decl.type,
							wrappedInit
						);
					});

					if (!changed) {
						newStatements.push(visit(statement, true));
						continue;
					}

					const newDeclList = factory.updateVariableDeclarationList(statement.declarationList, newDecls);
					const newVarStmt = factory.updateVariableStatement(statement, statement.modifiers, newDeclList);
					newStatements.push(newVarStmt);
				}
				return factory.updateBlock(node, newStatements);
			}

			return ts.visitEachChild(node, (child) => visit(child, childInFunctionBody), context);
		}

		return (node) => {
			const visited = visit(node, false);
			if (ts.isSourceFile(visited) && fileLevelHelpers.length > 0) {
				return factory.updateSourceFile(visited, [
					...fileLevelHelpers,
					...visited.statements,
				]);
			}
			return visited;
		};
	};

	const result = ts.transform(sourceFile, [transformer]);
	const transformed = result.transformed[0];
	const printer = ts.createPrinter({ newLine: ts.NewLineKind.LineFeed });
	const code = printer.printFile(transformed);
	result.dispose();

	return {
		code,
		transformedCount,
		transformedLocals,
	};
}

function runMetadata(outFile, metadataOutDir) {
	const tsToMetadataScript = path.join(process.cwd(), 'scripts', 'ts_to_metadata.js');
	if (!existsSync(tsToMetadataScript)) {
		throw new Error(`未找到脚本: ${tsToMetadataScript}`);
	}
	const args = [tsToMetadataScript, outFile];
	if (metadataOutDir) {
		args.push('--outDir', path.resolve(metadataOutDir));
	}
	execFileSync('node', args, { stdio: 'inherit' });
}

function main() {
	const args = parseArgs(process.argv.slice(2));
	const absInput = path.resolve(args.tsFile);
	if (!existsSync(absInput)) {
		throw new Error(`TS 文件不存在: ${absInput}`);
	}

	const outFile = args.outFile ? path.resolve(args.outFile) : resolveDefaultOutFile(absInput);
	const outDir = path.dirname(outFile);
	if (!existsSync(outDir)) mkdirSync(outDir, { recursive: true });

	const sourceText = readFileSync(absInput, 'utf8');
	const sourceFile = ts.createSourceFile(absInput, sourceText, ts.ScriptTarget.ES2020, true, ts.ScriptKind.TS);

	const program = ts.createProgram([absInput], {
		target: ts.ScriptTarget.ES2020,
		module: ts.ModuleKind.None,
		skipLibCheck: true,
		noEmit: true,
	});
	const checker = program.getTypeChecker();
	const sfFromProgram = program.getSourceFile(absInput) || sourceFile;

	const { code, transformedCount, transformedLocals } = transformFile(sfFromProgram, checker);
	writeFileSync(outFile, code, 'utf8');

	console.log(`✅ 预处理完成: ${outFile}`);
	console.log(`   - 注入包装函数数量: ${transformedCount}`);
	if (transformedLocals.length > 0) {
		for (const item of transformedLocals.slice(0, 20)) {
			console.log(`   - ${item.name}: ${item.type} -> ${item.wrapper}`);
		}
		if (transformedLocals.length > 20) {
			console.log(`   - ... 其余 ${transformedLocals.length - 20} 个省略`);
		}
	}

	if (args.runMetadata) {
		console.log('=== 继续执行 ts_to_metadata.js ===');
		runMetadata(outFile, args.metadataOutDir);
	}
}

try {
	main();
} catch (error) {
	console.error(`❌ ${error.message}`);
	process.exit(1);
}
