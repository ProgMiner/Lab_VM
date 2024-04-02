package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.NodeField;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;

@NodeField(name = "slot", type = int.class)
@NodeChild(value = "value", type = LamaExpr.class)
@NodeInfo(shortName = "=", description = "assignment in variable")
public abstract class AssignName extends LamaExpr {

    @Specialization
    protected long assignLong(VirtualFrame frame, long value) {
        frame.setLong(getSlot(), value);
        return value;
    }

    @Specialization
    protected Object assignObject(VirtualFrame frame, Object value) {
        frame.setObject(getSlot(), value);
        return value;
    }

    public abstract int getSlot();
}
