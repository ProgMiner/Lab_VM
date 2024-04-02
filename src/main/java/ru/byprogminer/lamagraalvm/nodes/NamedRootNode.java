package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import ru.byprogminer.lamagraalvm.LamaLanguage;

import java.util.Objects;

public class NamedRootNode extends LamaRootNode {

    private final String name;

    public NamedRootNode(
            LamaLanguage language,
            FrameDescriptor frameDescriptor,
            LamaExpr expr,
            int arguments,
            String name
    ) {
        super(language, frameDescriptor, expr, arguments);

        this.name = Objects.requireNonNull(name);
    }

    @Override
    public String getName() {
        return name;
    }
}
