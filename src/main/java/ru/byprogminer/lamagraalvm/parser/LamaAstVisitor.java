package ru.byprogminer.lamagraalvm.parser;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import lombok.RequiredArgsConstructor;
import org.antlr.v4.runtime.Token;
import ru.byprogminer.lamagraalvm.nodes.*;
import ru.byprogminer.lamagraalvm.nodes.builtin.Builtins;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RequiredArgsConstructor
public class LamaAstVisitor extends LamaBaseVisitor<LamaExpr> {

    private Scope currentScope = new Scope(null);

    public LamaAstVisitor(Builtins builtins) {
        builtins.initBuiltins(currentScope.frameDescriptorBuilder);

        for (final Builtins.BuiltinInfo b : builtins.getBuiltins()) {
            currentScope.slots.put(b.name(), b.slot());
        }
    }

    public FrameDescriptor createFrameDescriptor() {
        return currentScope.createFrameDescriptor();
    }

    @Override
    public LamaExpr visitProgram(LamaParser.ProgramContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExpr visitScopeExpr(LamaParser.ScopeExprContext ctx) {
        new PopulateScopeVisitor(currentScope).visitScopeExpr(ctx);

        LamaExpr result = null;
        for (final LamaParser.DefinitionContext def : ctx.definition()) {
            result = concat(result, visit(def));
        }

        final LamaParser.ExprContext expr = ctx.expr();
        if (expr != null) {
            result = concat(result, visitExpr(expr));
        }

        return ensureNotNull(result);
    }

    @Override
    public LamaExpr visitVarDef(LamaParser.VarDefContext ctx) {
        LamaExpr result = null;

        for (final LamaParser.VarDefItemContext item : ctx.varDefItem()) {
            result = concat(result, visitVarDefItem(item));
        }

        return result;
    }

    @Override
    public LamaExpr visitFunDef(LamaParser.FunDefContext ctx) {
        throw new UnsupportedOperationException("fun def");
    }

    @Override
    public LamaExpr visitVarDefItem(LamaParser.VarDefItemContext ctx) {
        final LamaParser.BasicExprContext value = ctx.basicExpr();

        if (value == null) {
            return null;
        }

        // TODO assign
        return visitBasicExpr(value);
    }

    @Override
    public LamaExpr visitExpr(LamaParser.ExprContext ctx) {
        LamaExpr result = null;

        for (final LamaParser.BasicExprContext expr : ctx.basicExpr()) {
            result = concat(result, visitBasicExpr(expr));
        }

        return result;
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
        final LamaExpr lhs = visit(ctx.lhs);
        final LamaExpr rhs = visit(ctx.rhs);

        return switch (ctx.op.getText()) {
            case "*" -> BinaryOperatorFactory.MultiplyFactory.create(lhs, rhs);
            case "/" -> BinaryOperatorFactory.DivideFactory.create(lhs, rhs);
            case "%" -> BinaryOperatorFactory.RemainderFactory.create(lhs, rhs);
            case "+" -> BinaryOperatorFactory.PlusFactory.create(lhs, rhs);
            case "-" -> BinaryOperatorFactory.MinusFactory.create(lhs, rhs);
            case "==" -> BinaryOperatorFactory.EqualsFactory.create(lhs, rhs);
            case "!=" -> BinaryOperatorFactory.NotEqualsFactory.create(lhs, rhs);
            case "<" -> BinaryOperatorFactory.LessFactory.create(lhs, rhs);
            case "<=" -> BinaryOperatorFactory.LessOrEqualFactory.create(lhs, rhs);
            case ">" -> BinaryOperatorFactory.GreaterFactory.create(lhs, rhs);
            case ">=" -> BinaryOperatorFactory.GreaterOrEqualFactory.create(lhs, rhs);
            case "&&" -> BinaryOperatorFactory.AndFactory.create(lhs, rhs);
            case "!!" -> BinaryOperatorFactory.OrFactory.create(lhs, rhs);
            case ":" -> BinaryOperatorFactory.ConsFactory.create(lhs, rhs);
            default -> throw new UnsupportedOperationException("binary " + ctx.op);
        };
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
        final LamaExpr fun = visit(ctx.postfixExpr());

        final List<LamaParser.ExprContext> argsCtx = ctx.expr();
        final LamaExpr[] args = new LamaExpr[argsCtx.size()];

        for (int i = 0; i < args.length; ++i) {
            args[i] = visitExpr(argsCtx.get(i));
        }

        return new Call(fun, args);
    }

    @Override
    public LamaExpr visitPostfixExprSubscript(LamaParser.PostfixExprSubscriptContext ctx) {
        throw new UnsupportedOperationException("subscript");
    }

    @Override
    public LamaExpr visitPrimaryExprDecimal(LamaParser.PrimaryExprDecimalContext ctx) {
        return new Constant(Long.parseLong(ctx.DECIMAL().getText()));
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
        return NameNodeGen.create(currentScope.getSlot(ctx.LIDENT().getSymbol()));
    }

    @Override
    public LamaExpr visitPrimaryExprTrue(LamaParser.PrimaryExprTrueContext ctx) {
        return new Constant(1);
    }

    @Override
    public LamaExpr visitPrimaryExprFalse(LamaParser.PrimaryExprFalseContext ctx) {
        return new Constant(0);
    }

    @Override
    public LamaExpr visitPrimaryExprFun(LamaParser.PrimaryExprFunContext ctx) {
        throw new UnsupportedOperationException("fun value");
    }

    @Override
    public LamaExpr visitPrimaryExprSkip(LamaParser.PrimaryExprSkipContext ctx) {
        return new Constant(0);
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

    private static LamaExpr concat(LamaExpr lhs, LamaExpr rhs) {
        if (lhs == null) {
            return rhs;
        }

        if (rhs == null) {
            return lhs;
        }

        if (lhs instanceof final Seq seq) {
            return new Seq(seq.getLhs(), concat(seq.getRhs(), rhs));
        }

        return new Seq(lhs, rhs);
    }

    private static LamaExpr ensureNotNull(LamaExpr expr) {
        if (expr == null) {
            return new Constant(0);
        }

        return expr;
    }

    @RequiredArgsConstructor
    private static class Scope {

        private final Scope parent;

        private final Map<String, Integer> slots = new HashMap<>();

        final FrameDescriptor.Builder frameDescriptorBuilder = Call.createFrameDescriptorBuilder();

        private void createSlot(Token id) {
            final String name = id.getText();

            final int idx = frameDescriptorBuilder.addSlot(FrameSlotKind.Illegal, name, null);

            if (slots.put(name, idx) != null) {
                ThrowingErrorListener.throwException(id.getLine(), id.getCharPositionInLine(),
                        "id \"" + name + "\" is already defined in scope");
            }
        }

        private int getSlot(Token id) {
            final Integer result = slots.get(id.getText());

            if (result == null) {
                ThrowingErrorListener.throwException(id.getLine(), id.getCharPositionInLine(),
                        "id \"" + id.getText() + "\" is not defined");
            }

            return result;
        }

        private FrameDescriptor createFrameDescriptor() {
            return frameDescriptorBuilder.build();
        }
    }

    @RequiredArgsConstructor
    private static class PopulateScopeVisitor extends LamaBaseVisitor<Void> {

        private final Scope scope;

        @Override
        public Void visitScopeExpr(LamaParser.ScopeExprContext ctx) {
            for (final LamaParser.DefinitionContext def : ctx.definition()) {
                visit(def);
            }

            return null;
        }

        @Override
        public Void visitVarDefItem(LamaParser.VarDefItemContext ctx) {
            scope.createSlot(ctx.LIDENT().getSymbol());
            return null;
        }

        @Override
        public Void visitFunDef(LamaParser.FunDefContext ctx) {
            scope.createSlot(ctx.LIDENT().getSymbol());
            return null;
        }
    }
}
