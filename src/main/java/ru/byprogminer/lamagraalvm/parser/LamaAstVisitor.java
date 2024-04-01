package ru.byprogminer.lamagraalvm.parser;

import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.ast.IntLiteral;
import ru.byprogminer.lamagraalvm.ast.LamaExpr;

@RequiredArgsConstructor
public class LamaAstVisitor extends LamaBaseVisitor<LamaExpr> {

    private final LamaLanguage language;

    @Override
    public LamaExpr visitExpr(LamaParser.ExprContext ctx) {
        return new IntLiteral(language, Long.parseLong(ctx.INT().getText()));
    }
}
