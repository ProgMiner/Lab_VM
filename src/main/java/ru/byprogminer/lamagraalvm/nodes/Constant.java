package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.EqualsAndHashCode;
import lombok.Value;

@Value
@EqualsAndHashCode(callSuper = true)
@NodeInfo(shortName = "const", description = "constant long value expression")
public class Constant extends LamaExpr {

    long value;

    @Override
    public long executeLong(VirtualFrame frame) {
        return value;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return value;
    }
}
