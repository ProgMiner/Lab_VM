package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.CallTarget;
import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.source.Source;
import lombok.NoArgsConstructor;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import ru.byprogminer.lamagraalvm.ast.LamaExpr;
import ru.byprogminer.lamagraalvm.parser.*;

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

    @Override
    protected LamaContext createContext(Env env) {
        return new LamaContext();
    }

    @Override
    protected CallTarget parse(ParsingRequest request) {
        final LamaLexer lexer = new LamaLexer(sourceToCharStream(request.getSource()));
        lexer.removeErrorListeners();
        lexer.addErrorListener(ThrowingErrorListener.INSTANCE);

        final LamaParser parser = new LamaParser(new CommonTokenStream(lexer));
        parser.removeErrorListeners();
        parser.addErrorListener(ThrowingErrorListener.INSTANCE);

        final LamaParser.ExprContext exprCtx = parser.expr();

        final LamaVisitor<LamaExpr> visitor = new LamaAstVisitor(this);
        final LamaExpr expr = visitor.visitExpr(exprCtx);
        return expr.getCallTarget();
    }

    private static CharStream sourceToCharStream(Source src) {
        return CharStreams.fromString(src.getCharacters().toString(), src.getPath());
    }
}
