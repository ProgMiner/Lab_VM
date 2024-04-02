package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.dsl.GenerateNodeFactory;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

@GenerateNodeFactory
@NodeInfo(shortName = "length", description = "length() builtin")
public abstract class LengthBuiltin extends Builtin {

    @Specialization
    public long length(LamaString value) {
        return value.value().length;
    }

    @Specialization
    public long length(LamaArray value) {
        return value.values().length;
    }

    @Specialization
    public long length(LamaSexp value) {
        return value.values().length;
    }

    @Specialization
    public long length(LamaFun value) {
        return value.parentScope().getFrameDescriptor().getNumberOfSlots();
    }
}
