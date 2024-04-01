package ru.byprogminer.lamagraalvm.expr;

import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.frame.VirtualFrame;

public final class IntLiteral extends LamaExpr {

    private final long value;

    public IntLiteral(TruffleLanguage<?> language, long value) {
        super(language);

        this.value = value;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return value;
    }
}
