package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.profiles.ConditionProfile;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@RequiredArgsConstructor
@NodeInfo(shortName = "if", description = "if expression")
public class If extends LamaExpr {

    @Child @NonNull private LamaExpr cond;
    @Child @NonNull private LamaExpr thenBranch;
    @Child @NonNull private LamaExpr elseBranch;

    private final ConditionProfile conditionProfile = ConditionProfile.create();

    @Override
    public Object execute(VirtualFrame frame) {
        if (conditionProfile.profile(executeCond(frame))) {
            return thenBranch.execute(frame);
        } else {
            return elseBranch.execute(frame);
        }
    }

    private boolean executeCond(VirtualFrame frame) {
        return executeCond(frame, cond);
    }
}
