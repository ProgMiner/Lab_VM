package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.NodeFactory;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.nodes.*;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;

import java.util.List;
import java.util.function.Consumer;

@NodeChild(value = "arguments", type = LamaExpr[].class)
@NodeInfo(shortName = "builtin", description = "builtin function node")
public abstract class Builtin extends LamaExpr {

    public static <T extends Builtin> LamaFun createBuiltin(
            LamaLanguage language,
            String name,
            NodeFactory<T> factory,
            Consumer<T> processor
    ) {
        final List<Class<? extends Node>> signature = factory.getExecutionSignature();

        final FrameDescriptor.Builder descriptorBuilder = FrameDescriptor.newBuilder();
        final LamaExpr[] args = new LamaExpr[signature.size()];
        for (int i = 0; i < args.length; i++) {
            descriptorBuilder.addSlot(FrameSlotKind.Illegal, null, null);
            args[i] = NameNodeGen.create(i);
        }

        final T node = factory.createNode((Object) args);
        processor.accept(node);

        final LamaRootNode rootNode = new NamedRootNode(language, descriptorBuilder.build(), node, args.length, name);
        return new LamaFun(rootNode.getCallTarget(), null);
    }

    public static <T extends Builtin> LamaFun createBuiltin(
            LamaLanguage language,
            String name,
            NodeFactory<T> factory
    ) {
        return createBuiltin(language, name, factory, t -> {});
    }
}
