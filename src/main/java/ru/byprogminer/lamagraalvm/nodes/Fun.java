package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;

@RequiredArgsConstructor
@NodeInfo(shortName = "fun", description = "function expression")
public class Fun extends LamaExpr {

    @NonNull private final RootCallTarget callTarget;

    @Override
    public Object execute(VirtualFrame frame) {
        return new LamaFun(callTarget, frame.materialize());
    }
}
