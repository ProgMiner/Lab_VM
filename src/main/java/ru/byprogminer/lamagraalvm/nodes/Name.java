package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeField;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameSlotTypeException;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;

@NodeField(name = "slot", type = int.class)
@NodeField(name = "depth", type = int.class)
@NodeInfo(shortName = "name", description = "read value of variable")
public abstract class Name extends LamaExpr {

    @Specialization(rewriteOn = FrameSlotTypeException.class)
    protected long readLong(VirtualFrame virtualFrame) throws FrameSlotTypeException {
        return getFrame(virtualFrame).getLong(getSlot());
    }

    @Specialization(rewriteOn = FrameSlotTypeException.class)
    protected Object readObject(VirtualFrame virtualFrame) throws FrameSlotTypeException {
        return getFrame(virtualFrame).getObject(getSlot());
    }

    @Specialization(replaces = {"readLong", "readObject"})
    protected Object read(VirtualFrame virtualFrame) {
        try {
            return getFrame(virtualFrame).getValue(getSlot());
        } catch (FrameSlotTypeException e) {
            // not reachable

            throw new RuntimeException(e);
        }
    }

    private Frame getFrame(Frame frame) {
        return getFrame(frame, getDepth());
    }

    public abstract int getSlot();

    public abstract int getDepth();
}
