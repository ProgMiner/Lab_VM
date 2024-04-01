package ru.byprogminer.lamagraalvm.ast;

import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.nodes.RootNode;

public sealed abstract class LamaExpr extends RootNode permits Constant {

    protected LamaExpr(TruffleLanguage<?> language) {
        super(language);
    }
}
