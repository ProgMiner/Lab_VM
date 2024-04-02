package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.dsl.TypeSystemReference;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.UnexpectedResultException;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.LamaTypes;
import ru.byprogminer.lamagraalvm.LamaTypesGen;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;
import ru.byprogminer.lamagraalvm.runtime.LamaRef;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;

@NodeInfo(
        shortName = "expr",
        language = LamaLanguage.NAME,
        description = "base expression node"
)
@TypeSystemReference(LamaTypes.class)
public abstract class LamaExpr extends Node {

    public long executeLong(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectLong(this.execute(frame));
    }

    public LamaRef executeRef(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectLamaRef(this.execute(frame));
    }

    public String executeString(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectString(this.execute(frame));
    }

    public LamaArray executeArray(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectLamaArray(this.execute(frame));
    }

    public LamaSexp executeSexp(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectLamaSexp(this.execute(frame));
    }

    public LamaFun executeFun(VirtualFrame frame) throws UnexpectedResultException {
        return LamaTypesGen.expectLamaFun(this.execute(frame));
    }

    public abstract Object execute(VirtualFrame frame);

    protected static boolean executeCond(VirtualFrame frame, LamaExpr cond) {
        try {
            return cond.executeLong(frame) != 0;
        } catch (UnexpectedResultException e) {
            CompilerDirectives.transferToInterpreterAndInvalidate();
            throw new IllegalArgumentException("condition must be integer");
        }
    }
}
