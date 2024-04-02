package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.NonNull;

@Data
@EqualsAndHashCode(callSuper = true)
@NodeInfo(shortName = ";", description = "sequential evaluation node")
public class Seq extends LamaExpr {

    @Child @NonNull LamaExpr lhs;
    @Child @NonNull LamaExpr rhs;

    // TODO specialize

    @Override
    public Object execute(VirtualFrame frame) {
        lhs.execute(frame);

        return rhs.execute(frame);
    }
}
