package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.CallTarget;
import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.source.Source;
import lombok.NoArgsConstructor;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import ru.byprogminer.lamagraalvm.nodes.LamaExpr;
import ru.byprogminer.lamagraalvm.nodes.LamaRootNode;
import ru.byprogminer.lamagraalvm.nodes.builtin.Builtins;
import ru.byprogminer.lamagraalvm.parser.LamaAstVisitor;
import ru.byprogminer.lamagraalvm.parser.LamaLexer;
import ru.byprogminer.lamagraalvm.parser.LamaParser;
import ru.byprogminer.lamagraalvm.parser.ThrowingErrorListener;

import java.util.Objects;

@TruffleLanguage.Registration(
        id = LamaLanguage.ID,
        name = LamaLanguage.NAME,
        defaultMimeType = LamaLanguage.MIME_TYPE,
        characterMimeTypes = LamaLanguage.MIME_TYPE,
        fileTypeDetectors = LamaFileTypeDetector.class,
        website = "https://github.com/PLTools/Lama/"
)
@NoArgsConstructor
public class LamaLanguage extends TruffleLanguage<LamaContext> {

    public static final String ID = "lama";

    public static final String NAME = "LaMa";

    public static final String MIME_TYPE = "application/x-lama";

    private LamaContext context;

    @Override
    protected LamaContext createContext(Env env) {
        return new LamaContext();
    }

    @Override
    protected void initializeContext(LamaContext context) {
        context.init();

        this.context = context;
    }

    @Override
    protected CallTarget parse(ParsingRequest request) {
        final LamaLexer lexer = new LamaLexer(sourceToCharStream(request.getSource()));
        lexer.removeErrorListeners();
        lexer.addErrorListener(ThrowingErrorListener.INSTANCE);

        final LamaParser parser = new LamaParser(new CommonTokenStream(lexer));
        parser.removeErrorListeners();
        parser.addErrorListener(ThrowingErrorListener.INSTANCE);

        final LamaParser.ProgramContext exprCtx = parser.program();

        final Builtins builtins = new Builtins(this, context.getOut(), context.getIn());

        final LamaAstVisitor visitor = new LamaAstVisitor(builtins);
        final LamaExpr expr = visitor.visitProgram(exprCtx).make(LamaAstVisitor.LamaExprSort.VAL);

        final LamaRootNode root = new MainRootNode(this, visitor.createFrameDescriptor(), expr, builtins);
        return root.getCallTarget();
    }

    private static CharStream sourceToCharStream(Source src) {
        return CharStreams.fromString(src.getCharacters().toString(), src.getPath());
    }

    private static class MainRootNode extends LamaRootNode {

        private final Builtins builtins;

        public MainRootNode(LamaLanguage language, FrameDescriptor frameDescriptor, LamaExpr expr, Builtins builtins) {
            super(language, frameDescriptor, expr, 0);

            this.builtins = Objects.requireNonNull(builtins);
        }

        @Override
        public Object execute(VirtualFrame frame) {
            builtins.populateFrame(frame);

            return super.execute(frame);
        }

        @Override
        public String getName() {
            return "<main>";
        }
    }
}
