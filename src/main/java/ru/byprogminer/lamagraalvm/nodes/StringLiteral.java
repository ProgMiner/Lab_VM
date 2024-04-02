package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.EqualsAndHashCode;
import lombok.Value;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

@Value
@EqualsAndHashCode(callSuper = true)
@NodeInfo(shortName = "string", description = "string literal")
public class StringLiteral extends LamaExpr {

    LamaString value;

    @Override
    public Object execute(VirtualFrame frame) {
        return value.clone();
    }
}
