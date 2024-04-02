package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.CompilerAsserts;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.NonNull;
import lombok.SneakyThrows;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;

@Data
@EqualsAndHashCode(callSuper = true)
@NodeInfo(shortName = "call", description = "function call")
public class Call extends LamaExpr {

    @Child @NonNull LamaExpr fun;
    @Children @NonNull LamaExpr[] args;

    @SneakyThrows
    @Override
    public Object execute(VirtualFrame frame) {
        CompilerAsserts.compilationConstant(args.length);

        final LamaFun fun = this.fun.executeFun(frame);
        final Object[] args = new Object[this.args.length + 1];

        args[0] = fun.parentScope();
        for (int i = 0; i < this.args.length; i++) {
            args[i + 1] = this.args[i].execute(frame);
        }

        return fun.callTarget().call(args);
    }

    public static FrameDescriptor.Builder createFrameDescriptorBuilder() {
        final FrameDescriptor.Builder result = FrameDescriptor.newBuilder().defaultValue(0L);

        result.addSlot(FrameSlotKind.Object, "parent scope", null);
        return result;
    }
}
