package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.runtime.LamaRef;

@NodeChild(value = "ref", type = LamaExpr.class)
@NodeChild(value = "value", type = LamaExpr.class)
@NodeInfo(shortName = ":=", description = "assignment in reference")
public abstract class Assign extends LamaExpr {

    @Specialization
    protected long assignLong(VirtualFrame frame, LamaRef ref, long value) {
        ref.assignLong(frame, value);
        return value;
    }

    @Specialization
    protected Object assignObject(VirtualFrame frame, LamaRef ref, Object value) {
        ref.assignObject(frame, value);
        return value;
    }
}
