package ru.byprogminer.lamagraalvm.expr;

import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.nodes.RootNode;

public sealed abstract class LamaExpr extends RootNode permits IntLiteral {

    protected LamaExpr(TruffleLanguage<?> language) {
        super(language);
    }
}
