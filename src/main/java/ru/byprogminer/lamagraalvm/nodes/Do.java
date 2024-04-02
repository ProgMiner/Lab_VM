package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.RepeatingNode;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@NodeInfo(shortName = "do", description = "do-while loop")
public class Do extends LamaLoopNode {

    public Do(LamaExpr body, LamaExpr cond) {
        super(new DoRepeatingNode(body, cond));
    }

    @RequiredArgsConstructor
    private static class DoRepeatingNode extends Node implements RepeatingNode {

        @Child @NonNull private LamaExpr body;
        @Child @NonNull private LamaExpr cond;

        @Override
        public boolean executeRepeating(VirtualFrame frame) {
            body.execute(frame);

            return executeCond(frame, cond);
        }
    }
}
