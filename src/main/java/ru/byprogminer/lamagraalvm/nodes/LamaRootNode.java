package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.source.SourceSection;
import lombok.Setter;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.runtime.LamaException;

import java.util.Objects;

@NodeInfo(language = LamaLanguage.NAME, description = "root node of LaMa program")
public class LamaRootNode extends RootNode {

    @Child private LamaExpr expr;

    private final int arguments;

    @Setter
    private SourceSection sourceSection;

    public LamaRootNode(LamaLanguage language, FrameDescriptor frameDescriptor, LamaExpr expr, int arguments) {
        super(language, frameDescriptor);

        this.expr = Objects.requireNonNull(expr);
        this.arguments = arguments;
    }

    @Override
    public SourceSection getSourceSection() {
        return sourceSection;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        final Object[] args = frame.getArguments();

        if (args.length < arguments) {
            CompilerDirectives.transferToInterpreterAndInvalidate();
            throw exn(new IllegalArgumentException("not enough arguments (expected " + arguments
                    + ", passed " + args.length + ")"));
        }

        for (int i = 0; i < arguments; ++i) {
            frame.setObject(i, args[i + 1]);
        }

        return expr.execute(frame);
    }

    protected LamaException exn(Throwable cause) {
        return new LamaException(cause, this);
    }
}
