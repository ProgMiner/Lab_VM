package ru.byprogminer.lamagraalvm.parser;

import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.ast.Constant;
import ru.byprogminer.lamagraalvm.ast.LamaExpr;

@RequiredArgsConstructor
public class LamaAstVisitor extends LamaBaseVisitor<LamaExpr> {

    private final LamaLanguage language;

    @Override
    public LamaExpr visitProgram(LamaParser.ProgramContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExpr visitScopeExpr(LamaParser.ScopeExprContext ctx) {
        if (!ctx.definition().isEmpty()) {
            throw new UnsupportedOperationException("definition");
        }

        final LamaParser.ExprContext expr = ctx.expr();
        if (expr == null) {
            return new Constant(language, 0);
        }

        return visitExpr(expr);
    }

    @Override
    public LamaExpr visitExpr(LamaParser.ExprContext ctx) {
        if (ctx.exprs.size() > 1) {
            throw new UnsupportedOperationException("seq");
        }

        return visitBasicExpr(ctx.exprs.get(0));
    }

    @Override
    public LamaExpr visitBasicExpr(LamaParser.BasicExprContext ctx) {
        return visit(ctx.binaryExpr());
    }

    @Override
    public LamaExpr visitBinaryExpr1(LamaParser.BinaryExpr1Context ctx) {
        return visitBinaryOperand(ctx.binaryOperand());
    }

    @Override
    public LamaExpr visitBinaryExpr2(LamaParser.BinaryExpr2Context ctx) {
        throw new UnsupportedOperationException("binary");
    }

    @Override
    public LamaExpr visitBinaryOperand(LamaParser.BinaryOperandContext ctx) {
        if (ctx.getChildCount() > 1) {
            throw new UnsupportedOperationException("unary minus");
        }

        return visit(ctx.postfixExpr());
    }

    @Override
    public LamaExpr visitPostfixExprPrimary(LamaParser.PostfixExprPrimaryContext ctx) {
        return visit(ctx.primaryExpr());
    }

    @Override
    public LamaExpr visitPostfixExprCall(LamaParser.PostfixExprCallContext ctx) {
        throw new UnsupportedOperationException("call");
    }

    @Override
    public LamaExpr visitPostfixExprSubscript(LamaParser.PostfixExprSubscriptContext ctx) {
        throw new UnsupportedOperationException("subscript");
    }

    @Override
    public LamaExpr visitPrimaryExprDecimal(LamaParser.PrimaryExprDecimalContext ctx) {
        return new Constant(language, Long.parseLong(ctx.DECIMAL().getText()));
    }

    @Override
    public LamaExpr visitPrimaryExprString(LamaParser.PrimaryExprStringContext ctx) {
        throw new UnsupportedOperationException("string");
    }

    @Override
    public LamaExpr visitPrimaryExprChar(LamaParser.PrimaryExprCharContext ctx) {
        throw new UnsupportedOperationException("char");
    }

    @Override
    public LamaExpr visitPrimaryExprId(LamaParser.PrimaryExprIdContext ctx) {
        throw new UnsupportedOperationException("id");
    }

    @Override
    public LamaExpr visitPrimaryExprTrue(LamaParser.PrimaryExprTrueContext ctx) {
        return new Constant(language, 1);
    }

    @Override
    public LamaExpr visitPrimaryExprFalse(LamaParser.PrimaryExprFalseContext ctx) {
        return new Constant(language, 0);
    }

    @Override
    public LamaExpr visitPrimaryExprFun(LamaParser.PrimaryExprFunContext ctx) {
        throw new UnsupportedOperationException("fun value");
    }

    @Override
    public LamaExpr visitPrimaryExprSkip(LamaParser.PrimaryExprSkipContext ctx) {
        return new Constant(language, 0);
    }

    @Override
    public LamaExpr visitPrimaryExprScope(LamaParser.PrimaryExprScopeContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExpr visitPrimaryExprList(LamaParser.PrimaryExprListContext ctx) {
        throw new UnsupportedOperationException("list");
    }

    @Override
    public LamaExpr visitPrimaryExprArray(LamaParser.PrimaryExprArrayContext ctx) {
        throw new UnsupportedOperationException("array");
    }

    @Override
    public LamaExpr visitPrimaryExprSexp(LamaParser.PrimaryExprSexpContext ctx) {
        throw new UnsupportedOperationException("sexp");
    }

    @Override
    public LamaExpr visitPrimaryExprIf(LamaParser.PrimaryExprIfContext ctx) {
        throw new UnsupportedOperationException("if");
    }

    @Override
    public LamaExpr visitPrimaryExprWhile(LamaParser.PrimaryExprWhileContext ctx) {
        throw new UnsupportedOperationException("while");
    }

    @Override
    public LamaExpr visitPrimaryExprDo(LamaParser.PrimaryExprDoContext ctx) {
        throw new UnsupportedOperationException("do");
    }

    @Override
    public LamaExpr visitPrimaryExprFor(LamaParser.PrimaryExprForContext ctx) {
        throw new UnsupportedOperationException("for");
    }
}
