package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import com.oracle.truffle.api.nodes.RootNode;
import ru.byprogminer.lamagraalvm.LamaLanguage;

import java.util.Objects;

@NodeInfo(language = LamaLanguage.NAME, description = "root node of LaMa program")
public class LamaRootNode extends RootNode {

    @Child private LamaExpr expr;

    public LamaRootNode(LamaLanguage language, FrameDescriptor frameDescriptor, LamaExpr expr) {
        super(language, frameDescriptor);

        this.expr = Objects.requireNonNull(expr);
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return expr.execute(frame);
    }
}
