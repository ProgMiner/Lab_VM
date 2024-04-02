package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;

import java.io.BufferedReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

@RequiredArgsConstructor
public class Builtins {

    @NonNull private final LamaLanguage language;

    @NonNull private final PrintWriter out;
    @NonNull private final BufferedReader in;

    private FrameDescriptor.Builder descriptorBuilder;

    private final List<BuiltinInfo> builtins = new ArrayList<>();

    public void initBuiltins(FrameDescriptor.Builder builder) {
        this.descriptorBuilder = Objects.requireNonNull(builder);

        registerBuiltin(Builtin.createBuiltin(language, "read", ReadBuiltinFactory.getInstance(), b -> {
            b.setOut(out);
            b.setIn(in);
        }));

        registerBuiltin(Builtin.createBuiltin(language, "write", WriteBuiltinFactory.getInstance(), b ->
                b.setOut(out)));

        this.descriptorBuilder = null;
    }

    public void populateFrame(Frame frame) {
        for (final BuiltinInfo b : builtins) {
            frame.setObject(b.slot, b.fun);
        }
    }

    public List<BuiltinInfo> getBuiltins() {
        return Collections.unmodifiableList(builtins);
    }

    private void registerBuiltin(LamaFun fun) {
        final String name = fun.callTarget().getRootNode().getName();

        builtins.add(new BuiltinInfo(name, descriptorBuilder.addSlot(FrameSlotKind.Object, name, null), fun));
    }

    public record BuiltinInfo(String name, int slot, LamaFun fun) {}
}
