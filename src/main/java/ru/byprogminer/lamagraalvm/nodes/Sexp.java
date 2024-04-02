package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;

@RequiredArgsConstructor
@NodeInfo(shortName = "sexp", description = "sexp literal")
public class Sexp extends LamaExpr {

    private final String tag;
    @Children @NonNull LamaExpr @NonNull[] values;

    @Override
    public Object execute(VirtualFrame frame) {
        final Object[] result = new Object[values.length];

        for (int i = 0; i < result.length; ++i) {
            result[i] = values[i].execute(frame);
        }

        return new LamaSexp(tag, result);
    }
}
