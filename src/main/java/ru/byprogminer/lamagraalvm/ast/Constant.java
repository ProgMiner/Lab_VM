package ru.byprogminer.lamagraalvm.ast;

import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.frame.VirtualFrame;

public final class Constant extends LamaExpr {

    private final long value;

    public Constant(TruffleLanguage<?> language, long value) {
        super(language);

        this.value = value;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return value;
    }
}
