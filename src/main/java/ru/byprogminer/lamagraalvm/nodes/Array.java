package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;

@RequiredArgsConstructor
@NodeInfo(shortName = "array", description = "array literal")
public class Array extends LamaExpr {

    @Children @NonNull LamaExpr @NonNull[] values;

    @Override
    public Object execute(VirtualFrame frame) {
        final Object[] result = new Object[values.length];

        for (int i = 0; i < result.length; ++i) {
            result[i] = values[i].execute(frame);
        }

        return new LamaArray(result);
    }
}
