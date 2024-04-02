package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.UnexpectedResultException;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@RequiredArgsConstructor
@NodeInfo(shortName = "do", description = "do-while loop")
public class Do extends LamaExpr {

    @Child @NonNull private LamaExpr body;
    @Child @NonNull private LamaExpr cond;

    // TODO RepeatingNode

    @Override
    public Object execute(VirtualFrame frame) {
        body.execute(frame);

        if (!executeCond(frame)) {
            return 0L;
        }

        return execute(frame);
    }

    private boolean executeCond(VirtualFrame frame) {
        try {
            return cond.executeLong(frame) != 0;
        } catch (UnexpectedResultException e) {
            CompilerDirectives.transferToInterpreterAndInvalidate();
            throw new IllegalArgumentException("condition must be integer");
        }
    }
}
