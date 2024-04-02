package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.LoopNode;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.RepeatingNode;

@NodeInfo(shortName = "loop", description = "base loop node")
public abstract class LamaLoopNode extends LamaExpr {

    @Child private LoopNode loop;

    public LamaLoopNode(RepeatingNode node) {
        loop = Truffle.getRuntime().createLoopNode(node);
    }

    @Override
    public long executeLong(VirtualFrame frame) {
        loop.execute(frame);
        return 0L;
    }

    @Override
    public final Object execute(VirtualFrame frame) {
        return executeLong(frame);
    }
}
