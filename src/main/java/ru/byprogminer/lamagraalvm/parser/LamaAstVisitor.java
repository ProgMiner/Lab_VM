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
public class LamaAstVisitor extends LamaBaseVisitor<LamaAstVisitor.LamaExprFactory> {

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
    public LamaExprFactory visitProgram(LamaParser.ProgramContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExprFactory visitScopeExpr(LamaParser.ScopeExprContext ctx) {
        return sort -> {
            new PopulateScopeVisitor(currentScope).visitScopeExpr(ctx);

            LamaExpr result = null;
            for (final LamaParser.DefinitionContext def : ctx.definition()) {
                result = concat(result, visit(def).make(null));
            }

            final LamaParser.ExprContext expr = ctx.expr();
            if (expr != null) {
                result = concat(result, visitExpr(expr).make(sort));
            }

            return ensureNotNull(result);
        };
    }

    @Override
    public LamaExprFactory visitVarDef(LamaParser.VarDefContext ctx) {
        return sort -> {
            LamaExpr result = null;

            for (final LamaParser.VarDefItemContext item : ctx.varDefItem()) {
                result = concat(result, visitVarDefItem(item).make(null));
            }

            return result;
        };
    }

    @Override
    public LamaExprFactory visitFunDef(LamaParser.FunDefContext ctx) {
        throw new UnsupportedOperationException("fun def");
    }

    @Override
    public LamaExprFactory visitVarDefItem(LamaParser.VarDefItemContext ctx) {
        return sort -> {
            final LamaParser.BasicExprContext value = ctx.basicExpr();

            if (value == null) {
                return null;
            }

            return AssignNameNodeGen.create(
                    visitBasicExpr(value).make(LamaExprSort.VAL),
                    currentScope.getSlot(ctx.LIDENT().getSymbol())
            );
        };
    }

    @Override
    public LamaExprFactory visitExpr(LamaParser.ExprContext ctx) {
        return sort -> {
            LamaExpr result = null;

            final List<LamaParser.BasicExprContext> basicExpr = ctx.basicExpr();
            for (int i = 0, basicExprSize = basicExpr.size(); i < basicExprSize; i++) {
                final LamaExprFactory f = visitBasicExpr(basicExpr.get(i));

                result = concat(result, i == basicExprSize - 1 ? f.make(sort) : f.make(LamaExprSort.VAL));
            }

            return result;
        };
    }

    @Override
    public LamaExprFactory visitBasicExpr(LamaParser.BasicExprContext ctx) {
        return visit(ctx.binaryExpr());
    }

    @Override
    public LamaExprFactory visitBinaryExpr1(LamaParser.BinaryExpr1Context ctx) {
        return visitBinaryOperand(ctx.binaryOperand());
    }

    @Override
    public LamaExprFactory visitBinaryExpr2(LamaParser.BinaryExpr2Context ctx) {
        return sort -> {
            sort.assertVal();

            final LamaExpr lhs = visit(ctx.lhs).make(LamaExprSort.VAL);
            final LamaExpr rhs = visit(ctx.rhs).make(LamaExprSort.VAL);

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
        };
    }

    @Override
    public LamaExprFactory visitBinaryExprAssign(LamaParser.BinaryExprAssignContext ctx) {
        return sort -> {
            sort.assertVal();

            final LamaExpr lhs = visit(ctx.lhs).make(LamaExprSort.REF);
            final LamaExpr rhs = visit(ctx.rhs).make(LamaExprSort.VAL);

            return AssignNodeGen.create(lhs, rhs);
        };
    }

    @Override
    public LamaExprFactory visitBinaryOperand(LamaParser.BinaryOperandContext ctx) {
        if (ctx.getChildCount() > 1) {
            throw new UnsupportedOperationException("unary minus");
        }

        return visit(ctx.postfixExpr());
    }

    @Override
    public LamaExprFactory visitPostfixExprPrimary(LamaParser.PostfixExprPrimaryContext ctx) {
        return visit(ctx.primaryExpr());
    }

    @Override
    public LamaExprFactory visitPostfixExprCall(LamaParser.PostfixExprCallContext ctx) {
        return sort -> {
            sort.assertVal();

            final LamaExpr fun = visit(ctx.postfixExpr()).make(LamaExprSort.VAL);

            final List<LamaParser.ExprContext> argsCtx = ctx.expr();
            final LamaExpr[] args = new LamaExpr[argsCtx.size()];

            for (int i = 0; i < args.length; ++i) {
                args[i] = visitExpr(argsCtx.get(i)).make(LamaExprSort.VAL);
            }

            return new Call(fun, args);
        };
    }

    @Override
    public LamaExprFactory visitPostfixExprSubscript(LamaParser.PostfixExprSubscriptContext ctx) {
        throw new UnsupportedOperationException("subscript");
    }

    @Override
    public LamaExprFactory visitPrimaryExprDecimal(LamaParser.PrimaryExprDecimalContext ctx) {
        return sort -> {
            sort.assertVal();
            return new Constant(Long.parseLong(ctx.DECIMAL().getText()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprString(LamaParser.PrimaryExprStringContext ctx) {
        throw new UnsupportedOperationException("string");
    }

    @Override
    public LamaExprFactory visitPrimaryExprChar(LamaParser.PrimaryExprCharContext ctx) {
        throw new UnsupportedOperationException("char");
    }

    @Override
    public LamaExprFactory visitPrimaryExprId(LamaParser.PrimaryExprIdContext ctx) {
        return sort -> {
            final int slot = currentScope.getSlot(ctx.LIDENT().getSymbol());

            // TODO nested scopes

            return switch (sort) {
                case VAL -> NameNodeGen.create(slot);
                case REF -> NameRefNodeGen.create(slot);
            };
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprTrue(LamaParser.PrimaryExprTrueContext ctx) {
        return sort -> {
            sort.assertVal();
            return new Constant(1);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprFalse(LamaParser.PrimaryExprFalseContext ctx) {
        return sort -> {
            sort.assertVal();
            return new Constant(0);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprFun(LamaParser.PrimaryExprFunContext ctx) {
        throw new UnsupportedOperationException("fun value");
    }

    @Override
    public LamaExprFactory visitPrimaryExprSkip(LamaParser.PrimaryExprSkipContext ctx) {
        return sort -> {
            sort.assertVal();
            return new Constant(0);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprScope(LamaParser.PrimaryExprScopeContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExprFactory visitPrimaryExprList(LamaParser.PrimaryExprListContext ctx) {
        throw new UnsupportedOperationException("list");
    }

    @Override
    public LamaExprFactory visitPrimaryExprArray(LamaParser.PrimaryExprArrayContext ctx) {
        throw new UnsupportedOperationException("array");
    }

    @Override
    public LamaExprFactory visitPrimaryExprSexp(LamaParser.PrimaryExprSexpContext ctx) {
        throw new UnsupportedOperationException("sexp");
    }

    @Override
    public LamaExprFactory visitPrimaryExprIf(LamaParser.PrimaryExprIfContext ctx) {
        throw new UnsupportedOperationException("if");
    }

    @Override
    public LamaExprFactory visitPrimaryExprWhile(LamaParser.PrimaryExprWhileContext ctx) {
        throw new UnsupportedOperationException("while");
    }

    @Override
    public LamaExprFactory visitPrimaryExprDo(LamaParser.PrimaryExprDoContext ctx) {
        throw new UnsupportedOperationException("do");
    }

    @Override
    public LamaExprFactory visitPrimaryExprFor(LamaParser.PrimaryExprForContext ctx) {
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

    public enum LamaExprSort {

        VAL,

        REF {

            @Override
            void assertVal() {
                throw new IllegalArgumentException("cannot produce expression of sort REF");
            }
        };

        void assertVal() {}
    }

    @FunctionalInterface
    public interface LamaExprFactory {

        LamaExpr make(LamaExprSort sort);
    }

    @RequiredArgsConstructor
    private static class Scope {

        private final Scope parent;

        private final Map<String, Integer> slots = new HashMap<>();

        final FrameDescriptor.Builder frameDescriptorBuilder = FrameDescriptor.newBuilder()
                .defaultValue(0L);

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
