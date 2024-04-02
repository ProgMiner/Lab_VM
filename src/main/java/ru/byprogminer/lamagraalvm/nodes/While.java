package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@RequiredArgsConstructor
@NodeChild(value = "cond", type = LamaExpr.class)
@NodeInfo(shortName = "while", description = "while loop")
public abstract class While extends LamaExpr {

    @Child @NonNull private LamaExpr body;

    // TODO RepeatingNode

    @Specialization
    public Object exec(VirtualFrame frame, long cond) {
        if (cond == 0) {
            return 0L;
        }

        body.execute(frame);
        return execute(frame);
    }
}
