package ru.byprogminer.lamagraalvm.parser;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import lombok.RequiredArgsConstructor;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.misc.ParseCancellationException;
import org.antlr.v4.runtime.tree.TerminalNode;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.nodes.*;
import ru.byprogminer.lamagraalvm.nodes.builtin.Builtins;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

import java.nio.charset.StandardCharsets;
import java.util.*;

@RequiredArgsConstructor
public class LamaAstVisitor extends LamaBaseVisitor<LamaAstVisitor.LamaExprFactory> {

    public static final String LIST_TAG = "cons";

    private final LamaLanguage language;

    private Scope currentScope = new Scope();

    public LamaAstVisitor(LamaLanguage language, Builtins builtins) {
        this.language = language;

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
            final Scope prevScope = currentScope;
            currentScope = prevScope.createFlatScope();

            try {
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
            } finally {
                currentScope = prevScope;
            }
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
        return sort -> {
            final Token name = ctx.LIDENT().getSymbol();

            return AssignNameNodeGen.create(
                    createFun(ctx.funArgs(), ctx.scopeExpr(), name.getText()),
                    currentScope.getSlot(name)
            );
        };
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
            sort.assertVal(ctx);

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
                default -> throw makeException(ctx, "unknown binary operator " + ctx.op.getText());
            };
        };
    }

    @Override
    public LamaExprFactory visitBinaryExprAssign(LamaParser.BinaryExprAssignContext ctx) {
        return sort -> {
            sort.assertVal(ctx);

            final LamaExpr lhs = visit(ctx.lhs).make(LamaExprSort.REF);
            final LamaExpr rhs = visit(ctx.rhs).make(LamaExprSort.VAL);

            return AssignNodeGen.create(lhs, rhs);
        };
    }

    @Override
    public LamaExprFactory visitBinaryOperand(LamaParser.BinaryOperandContext ctx) {
        if (ctx.getChildCount() > 1) {
            throw makeException(ctx, "unary minus");
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
            sort.assertVal(ctx);

            final LamaExpr fun = visit(ctx.postfixExpr()).make(LamaExprSort.VAL);
            final LamaExpr[] args = exprListToArray(ctx.expr());

            return new Call(fun, args);
        };
    }

    @Override
    public LamaExprFactory visitPostfixExprSubscript(LamaParser.PostfixExprSubscriptContext ctx) {
        return sort -> {
            final LamaExpr value = visit(ctx.postfixExpr()).make(LamaExprSort.VAL);
            final LamaExpr index = visitExpr(ctx.expr()).make(LamaExprSort.VAL);

            return switch (sort) {
                case VAL -> SubscriptNodeGen.create(value, index);
                case REF -> SubscriptRefNodeGen.create(value, index);
            };
        };
    }

    @Override
    public LamaExprFactory visitPostfixExprDotCall(LamaParser.PostfixExprDotCallContext ctx) {
        return sort -> {
            sort.assertVal(ctx);

            final LamaExpr fun = createName(ctx.LIDENT().getSymbol(), LamaExprSort.VAL);

            final List<LamaParser.ExprContext> argsCtx = ctx.expr();
            final LamaExpr[] args = new LamaExpr[argsCtx.size() + 1];
            args[0] = visit(ctx.postfixExpr()).make(LamaExprSort.VAL);

            for (int i = 1; i < args.length; ++i) {
                args[i] = visitExpr(argsCtx.get(i - 1)).make(LamaExprSort.VAL);
            }

            return new Call(fun, args);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprDecimal(LamaParser.PrimaryExprDecimalContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Constant(Long.parseLong(ctx.DECIMAL().getText()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprString(LamaParser.PrimaryExprStringContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new StringLiteral(convertStringLiteral(ctx.STRING().getText()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprChar(LamaParser.PrimaryExprCharContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Constant(decodeCharLiteral(ctx.CHAR().getSymbol()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprId(LamaParser.PrimaryExprIdContext ctx) {
        return sort -> createName(ctx.LIDENT().getSymbol(), sort);
    }

    @Override
    public LamaExprFactory visitPrimaryExprTrue(LamaParser.PrimaryExprTrueContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Constant(1);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprFalse(LamaParser.PrimaryExprFalseContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Constant(0);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprFun(LamaParser.PrimaryExprFunContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return createFun(ctx.funArgs(), ctx.scopeExpr(), null);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprSkip(LamaParser.PrimaryExprSkipContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Constant(0);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprScope(LamaParser.PrimaryExprScopeContext ctx) {
        return visitScopeExpr(ctx.scopeExpr());
    }

    @Override
    public LamaExprFactory visitPrimaryExprList(LamaParser.PrimaryExprListContext ctx) {
        throw makeException(ctx, "list");
    }

    @Override
    public LamaExprFactory visitPrimaryExprArray(LamaParser.PrimaryExprArrayContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Array(exprListToArray(ctx.expr()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprSexp(LamaParser.PrimaryExprSexpContext ctx) {
        return sort -> {
            sort.assertVal(ctx);
            return new Sexp(ctx.UIDENT().getText(), exprListToArray(ctx.expr()));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprIf(LamaParser.PrimaryExprIfContext ctx) {
        return sort -> {
            final List<LamaParser.ExprContext> conds = ctx.conds;
            final List<LamaParser.ScopeExprContext> thens = ctx.thens;

            final List<LamaExpr[]> branches = new ArrayList<>();

            final int branchesSize = conds.size();
            for (int i = 0; i < branchesSize; ++i) {
                final LamaExpr cond = visitExpr(conds.get(i)).make(LamaExprSort.VAL);
                final LamaExpr then = visitScopeExpr(thens.get(i)).make(sort);

                branches.add(new LamaExpr[] { cond, then });
            }

            LamaExpr result;

            if (ctx.else_ == null) {
                sort.assertVal(ctx);
                result = new Constant(0);
            } else {
                result = visitScopeExpr(ctx.else_).make(sort);
            }

            for (int i = branchesSize - 1; i >= 0; --i) {
                final LamaExpr[] cb = branches.get(i);

                result = new If(cb[0], cb[1], result);
            }

            return result;
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprWhile(LamaParser.PrimaryExprWhileContext ctx) {
        return sort -> {
            sort.assertVal(ctx);

            final LamaExpr cond = visitExpr(ctx.cond).make(LamaExprSort.VAL);
            final LamaExpr body = visitScopeExpr(ctx.body).make(LamaExprSort.VAL);

            return new While(cond, body);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprDo(LamaParser.PrimaryExprDoContext ctx) {
        return sort -> {
            sort.assertVal(ctx);

            final LamaExpr body = visitScopeExpr(ctx.body).make(LamaExprSort.VAL);
            final LamaExpr cond = visitExpr(ctx.cond).make(LamaExprSort.VAL);

            return new Do(body, cond);
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprFor(LamaParser.PrimaryExprForContext ctx) {
        return sort -> {
            sort.assertVal(ctx);

            // TODO fix scopes

            final LamaExpr init = visitScopeExpr(ctx.init).make(LamaExprSort.VAL);
            final LamaExpr cond = visitExpr(ctx.cond).make(LamaExprSort.VAL);
            final LamaExpr body = visitScopeExpr(ctx.body).make(LamaExprSort.VAL);
            final LamaExpr post = visitExpr(ctx.post).make(LamaExprSort.VAL);

            return concat(init, new While(cond, concat(body, post)));
        };
    }

    @Override
    public LamaExprFactory visitPrimaryExprCase(LamaParser.PrimaryExprCaseContext ctx) {
        return sort -> {
            final LamaExpr value = visitExpr(ctx.expr()).make(LamaExprSort.VAL);

            final List<LamaParser.CaseBranchContext> branchCtx = ctx.caseBranch();

            final Case.Matcher[] matchers = new Case.Matcher[branchCtx.size()];
            final LamaExpr[] bodies = new LamaExpr[branchCtx.size()];

            final Scope prevScope = currentScope;

            for (int i = 0; i < branchCtx.size(); i++) {
                currentScope = prevScope.createFlatScope();

                try {
                    final LamaParser.CaseBranchContext br = branchCtx.get(i);
                    final LamaParser.PatternContext p = br.pattern();

                    new PopulateScopeVisitor(currentScope).visit(p);

                    matchers[i] = new MatcherVisitor(currentScope).visit(p);
                    bodies[i] = visitScopeExpr(br.scopeExpr()).make(sort);
                } finally {
                    currentScope = prevScope;
                }
            }

            return new Case(value, matchers, bodies);
        };
    }

    private LamaExpr createName(Token id, LamaExprSort sort) {
        final int[] depth = new int[] { 0 };

        final int slot = currentScope.getSlotRec(id, depth);

        return switch (sort) {
            case VAL -> NameNodeGen.create(slot, depth[0]);
            case REF -> NameRefNodeGen.create(slot, depth[0]);
        };
    }

    private LamaExpr createFun(LamaParser.FunArgsContext argsCtx, LamaParser.ScopeExprContext bodyCtx, String name) {
        final Scope scope = currentScope;

        final Scope funScope = scope.createScope();
        currentScope = funScope;

        try {
            List<TerminalNode> args = argsCtx.LIDENT();

            for (final TerminalNode arg : args) {
                currentScope.createSlot(arg.getSymbol());
            }

            final LamaExpr body = visitScopeExpr(bodyCtx).make(LamaExprSort.VAL);

            final FrameDescriptor frameDescriptor = funScope.createFrameDescriptor();
            final LamaRootNode rootNode;

            if (name == null) {
                rootNode = new LamaRootNode(language, frameDescriptor, body, args.size());
            } else {
                rootNode = new NamedRootNode(language, frameDescriptor, body, args.size(), name);
            }

            return new Fun(rootNode.getCallTarget());
        } finally {
            currentScope = scope;
        }
    }

    private LamaExpr[] exprListToArray(List<LamaParser.ExprContext> values) {
        final LamaExpr[] result = new LamaExpr[values.size()];

        for (int i = 0; i < result.length; ++i) {
            result[i] = visitExpr(values.get(i)).make(LamaExprSort.VAL);
        }

        return result;
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

    private static LamaString convertStringLiteral(String string) {
        final String plain = string.substring(1, string.length() - 1).replaceAll("\"\"", "\"");

        return new LamaString(plain.getBytes(StandardCharsets.UTF_8));
    }

    private static int decodeCharLiteral(Token token) {
        final String value = token.getText();

        final String str = value.substring(1, value.length() - 1);

        switch (str) {
            case "''": return '\'';
            case "\\n": return '\n';
            case "\\t": return '\t';
        }

        if (str.length() > 1) {
            throw makeException(token, "illegal char literal");
        }

        return str.charAt(0);
    }

    private static ParseCancellationException makeException(ParserRuleContext ctx, String msg) {
        return makeException(ctx.start, msg);
    }

    private static ParseCancellationException makeException(Token start, String msg) {
        return ThrowingErrorListener.makeException(start.getLine(), start.getCharPositionInLine(), msg);
    }

    public enum LamaExprSort {

        VAL {

            @Override
            void assertVal(ParserRuleContext ctx) {}
        },

        REF;

        void assertVal(ParserRuleContext ctx) {
            throw makeException(ctx, "expected expression of sort Val");
        }
    }

    @FunctionalInterface
    public interface LamaExprFactory {

        LamaExpr make(LamaExprSort sort);
    }

    private static class Scope {

        private final Scope parent;

        private final int frameDepth;

        private final Map<String, Integer> slots = new HashMap<>();

        private final FrameDescriptor.Builder frameDescriptorBuilder;

        public Scope() {
            this(null);
        }

        private Scope(Scope parent) {
            this(parent, 1, FrameDescriptor.newBuilder().defaultValue(0L));
        }

        private Scope(Scope parent, int frameDepth, FrameDescriptor.Builder frameDescriptorBuilder) {
            this.parent = parent;
            this.frameDepth = frameDepth;
            this.frameDescriptorBuilder = Objects.requireNonNull(frameDescriptorBuilder);
        }

        public void createSlot(Token id) {
            final String name = id.getText();

            final int idx = frameDescriptorBuilder.addSlot(FrameSlotKind.Illegal, name, null);

            if (slots.put(name, idx) != null) {
                throw makeException(id, "id \"" + name + "\" is already defined in scope");
            }
        }

        public int getSlot(Token id) {
            final Integer result = slots.get(id.getText());

            if (result == null) {
                throw makeException(id, "id \"" + id.getText() + "\" is not defined");
            }

            return result;
        }

        public int getSlotRec(Token id, int[] depth) {
            for (Scope scope = this; scope != null; scope = scope.parent) {
                final Integer result = scope.slots.get(id.getText());

                if (result != null) {
                    return result;
                }

                depth[0] += scope.frameDepth;
            }

            throw makeException(id, "id \"" + id.getText() + "\" is not defined");
        }

        public FrameDescriptor createFrameDescriptor() {
            return frameDescriptorBuilder.build();
        }

        public Scope createScope() {
            return new Scope(this);
        }

        public Scope createFlatScope() {
            return new Scope(this, 0, this.frameDescriptorBuilder);
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

        @Override
        public Void visitPatternCons(LamaParser.PatternConsContext ctx) {
            return visitConsPattern(ctx.consPattern());
        }

        @Override
        public Void visitPatternSimple(LamaParser.PatternSimpleContext ctx) {
            return visit(ctx.simplePattern());
        }

        @Override
        public Void visitConsPattern(LamaParser.ConsPatternContext ctx) {
            while (true) {
                visit(ctx.simplePattern());

                final LamaParser.PatternContext tail = ctx.pattern();

                if (tail instanceof LamaParser.PatternConsContext tailCons) {
                    ctx = tailCons.consPattern();
                    continue;
                }

                if (tail instanceof LamaParser.PatternSimpleContext tailSimple) {
                    return visit(tailSimple.simplePattern());
                }

                throw makeException(ctx, "unknown type of pattern");
            }
        }

        @Override
        public Void visitSimplePatternWildcard(LamaParser.SimplePatternWildcardContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternSexp(LamaParser.SimplePatternSexpContext ctx) {
            return visitPatterns(ctx.pattern());
        }

        @Override
        public Void visitSimplePatternArray(LamaParser.SimplePatternArrayContext ctx) {
            return visitPatterns(ctx.pattern());
        }

        @Override
        public Void visitSimplePatternList(LamaParser.SimplePatternListContext ctx) {
            return visitPatterns(ctx.pattern());
        }

        @Override
        public Void visitSimplePatternNamed(LamaParser.SimplePatternNamedContext ctx) {
            final LamaParser.PatternContext p = ctx.pattern();

            scope.createSlot(ctx.LIDENT().getSymbol());

            if (p != null) {
                visit(p);
            }

            return null;
        }

        @Override
        public Void visitSimplePatternDecimal(LamaParser.SimplePatternDecimalContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternString(LamaParser.SimplePatternStringContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternChar(LamaParser.SimplePatternCharContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternTrue(LamaParser.SimplePatternTrueContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternFalse(LamaParser.SimplePatternFalseContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternBoxShape(LamaParser.SimplePatternBoxShapeContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternValShape(LamaParser.SimplePatternValShapeContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternStrShape(LamaParser.SimplePatternStrShapeContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternArrShape(LamaParser.SimplePatternArrShapeContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternFunShape(LamaParser.SimplePatternFunShapeContext ctx) {
            return null;
        }

        @Override
        public Void visitSimplePatternParens(LamaParser.SimplePatternParensContext ctx) {
            return visit(ctx.pattern());
        }

        private Void visitPatterns(Iterable<? extends LamaParser.PatternContext> ps) {
            for (final LamaParser.PatternContext p : ps) {
                visit(p);
            }

            return null;
        }
    }

    @RequiredArgsConstructor
    private static class MatcherVisitor extends LamaBaseVisitor<Case.Matcher> {

        private final Scope scope;

        @Override
        public Case.Matcher visitPatternCons(LamaParser.PatternConsContext ctx) {
            return visitConsPattern(ctx.consPattern());
        }

        @Override
        public Case.Matcher visitPatternSimple(LamaParser.PatternSimpleContext ctx) {
            return visit(ctx.simplePattern());
        }

        @Override
        public Case.Matcher visitConsPattern(LamaParser.ConsPatternContext ctx) {
            return Case.Matcher.sexp(LIST_TAG, Arrays.asList(visit(ctx.simplePattern()), visit(ctx.pattern())));
        }

        @Override
        public Case.Matcher visitSimplePatternWildcard(LamaParser.SimplePatternWildcardContext ctx) {
            return Case.Matcher.wildcard();
        }

        @Override
        public Case.Matcher visitSimplePatternSexp(LamaParser.SimplePatternSexpContext ctx) {
            return Case.Matcher.sexp(ctx.UIDENT().getText(), ctx.pattern().stream().map(this::visit).toList());
        }

        @Override
        public Case.Matcher visitSimplePatternArray(LamaParser.SimplePatternArrayContext ctx) {
            return Case.Matcher.array(ctx.pattern().stream().map(this::visit).toList());
        }

        @Override
        public Case.Matcher visitSimplePatternList(LamaParser.SimplePatternListContext ctx) {
            final List<Case.Matcher> ps = ctx.pattern().stream().map(this::visit).toList();

            Case.Matcher result = Case.Matcher.decimal(0);
            for (int i = ps.size() - 1; i >= 0; --i) {
                result = Case.Matcher.sexp(LIST_TAG, Arrays.asList(ps.get(i), result));
            }

            return result;
        }

        @Override
        public Case.Matcher visitSimplePatternNamed(LamaParser.SimplePatternNamedContext ctx) {
            final LamaParser.PatternContext p = ctx.pattern();

            final Case.Matcher m;
            if (p == null) {
                m = Case.Matcher.wildcard();
            } else {
                m = visit(p);
            }

            return Case.Matcher.named(scope.getSlot(ctx.LIDENT().getSymbol()), m);
        }

        @Override
        public Case.Matcher visitSimplePatternDecimal(LamaParser.SimplePatternDecimalContext ctx) {
            return Case.Matcher.decimal(Long.parseLong(ctx.getText()));
        }

        @Override
        public Case.Matcher visitSimplePatternString(LamaParser.SimplePatternStringContext ctx) {
            return Case.Matcher.string(convertStringLiteral(ctx.STRING().getText()));
        }

        @Override
        public Case.Matcher visitSimplePatternChar(LamaParser.SimplePatternCharContext ctx) {
            return Case.Matcher.decimal(decodeCharLiteral(ctx.CHAR().getSymbol()));
        }

        @Override
        public Case.Matcher visitSimplePatternTrue(LamaParser.SimplePatternTrueContext ctx) {
            return Case.Matcher.decimal(1);
        }

        @Override
        public Case.Matcher visitSimplePatternFalse(LamaParser.SimplePatternFalseContext ctx) {
            return Case.Matcher.decimal(0);
        }

        @Override
        public Case.Matcher visitSimplePatternBoxShape(LamaParser.SimplePatternBoxShapeContext ctx) {
            return Case.Matcher.boxShape();
        }

        @Override
        public Case.Matcher visitSimplePatternValShape(LamaParser.SimplePatternValShapeContext ctx) {
            return Case.Matcher.valShape();
        }

        @Override
        public Case.Matcher visitSimplePatternStrShape(LamaParser.SimplePatternStrShapeContext ctx) {
            return Case.Matcher.shape(LamaString.class);
        }

        @Override
        public Case.Matcher visitSimplePatternArrShape(LamaParser.SimplePatternArrShapeContext ctx) {
            return Case.Matcher.shape(LamaArray.class);
        }

        @Override
        public Case.Matcher visitSimplePatternFunShape(LamaParser.SimplePatternFunShapeContext ctx) {
            return Case.Matcher.shape(LamaFun.class);
        }

        @Override
        public Case.Matcher visitSimplePatternParens(LamaParser.SimplePatternParensContext ctx) {
            return visit(ctx.pattern());
        }
    }
}
