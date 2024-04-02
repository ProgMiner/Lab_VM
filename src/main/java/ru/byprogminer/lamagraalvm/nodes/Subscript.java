package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

@NodeChild(value = "value", type = LamaExpr.class)
@NodeChild(value = "index", type = LamaExpr.class)
@NodeInfo(shortName = "subscript", description = "subscript expression")
public abstract class Subscript extends LamaExpr {

    @Specialization
    protected long subscriptString(LamaString value, long index) {
        return value.value()[Math.toIntExact(index)];
    }

    @Specialization
    protected Object subscriptArray(LamaArray value, long index) {
        return value.values()[Math.toIntExact(index)];
    }

    @Specialization
    protected Object subscriptSexp(LamaSexp value, long index) {
        return value.values()[Math.toIntExact(index)];
    }
}
