package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.CallTarget;
import com.oracle.truffle.api.TruffleLanguage;
import lombok.NoArgsConstructor;
import ru.byprogminer.lamagraalvm.expr.IntLiteral;

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
        return new IntLiteral(this, 0).getCallTarget();
    }
}
