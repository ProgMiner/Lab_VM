package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeField;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.runtime.LamaRef;

@NodeField(name = "slot", type = int.class)
@NodeInfo(shortName = "name ref", description = "get reference to variable")
public abstract class NameRef extends LamaExpr {

    @Specialization
    protected LamaRef ref() {
        return new LamaRef() {

            @Override
            public void assignLong(VirtualFrame frame, long value) {
                frame.setLong(getSlot(), value);
            }

            @Override
            public void assignObject(VirtualFrame frame, Object value) {
                frame.setObject(getSlot(), value);
            }
        };
    }

    public abstract int getSlot();
}
