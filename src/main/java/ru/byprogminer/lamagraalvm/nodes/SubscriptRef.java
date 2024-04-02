package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaRef;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

@NodeChild(value = "value", type = LamaExpr.class)
@NodeChild(value = "index", type = LamaExpr.class)
@NodeInfo(shortName = "subscript", description = "subscript expression")
public abstract class SubscriptRef extends LamaExpr {

    @Specialization
    protected LamaRef subscriptString(LamaString value, long index) {
        final int intIndex = Math.toIntExact(index);

        return new LamaRef() {

            @Override
            public void assignLong(VirtualFrame frame, long x) {
                value.value()[intIndex] = (byte) x;
            }

            @Override
            public void assignObject(VirtualFrame frame, Object value) {
                if (!(value instanceof Long)) {
                    throw new UnsupportedOperationException("unable to assign object to string index");
                }

                assignLong(frame, (long) value);
            }
        };
    }

    @Specialization
    protected LamaRef subscriptArray(LamaArray value, long index) {
        return makeRef(value.values(), index);
    }

    @Specialization
    protected LamaRef subscriptSexp(LamaSexp value, long index) {
        return makeRef(value.values(), index);
    }

    private static LamaRef makeRef(Object[] xs, long index) {
        final int intIndex = Math.toIntExact(index);

        return new LamaRef() {

            @Override
            public void assignLong(VirtualFrame frame, long value) {
                assignObject(frame, value);
            }

            @Override
            public void assignObject(VirtualFrame frame, Object value) {
                xs[intIndex] = value;
            }
        };
    }
}
