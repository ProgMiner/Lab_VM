package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.RepeatingNode;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@NodeInfo(shortName = "while", description = "while loop")
public class While extends LamaLoopNode {

    public While(LamaExpr cond, LamaExpr body) {
        super(new WhileRepeatingNode(cond, body));
    }

    @RequiredArgsConstructor
    private static class WhileRepeatingNode extends Node implements RepeatingNode {

        @Child @NonNull private LamaExpr cond;
        @Child @NonNull private LamaExpr body;

        @Override
        public boolean executeRepeating(VirtualFrame frame) {
            if (!executeCond(frame, cond)) {
                return false;
            }

            body.execute(frame);
            return true;
        }
    }
}
