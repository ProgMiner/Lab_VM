package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.dsl.GenerateNodeFactory;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.MaterializedFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

@GenerateNodeFactory
@NodeInfo(shortName = "length", description = "length() builtin")
public abstract class StringBuiltin extends Builtin {

    @Specialization
    public LamaString string(long value) {
        final StringBuilder sb = new StringBuilder();
        string(value, sb);

        return LamaString.ofString(sb.toString());
    }

    @Specialization
    public LamaString string(LamaString value) {
        final StringBuilder sb = new StringBuilder();
        string(value, sb);

        return LamaString.ofString(sb.toString());
    }

    @Specialization
    public LamaString string(LamaArray value) {
        final StringBuilder sb = new StringBuilder();
        string(value, sb);

        return LamaString.ofString(sb.toString());
    }

    @Specialization
    public LamaString string(LamaSexp value) {
        final StringBuilder sb = new StringBuilder();
        string(value, sb);

        return LamaString.ofString(sb.toString());
    }

    @Specialization
    public LamaString string(LamaFun value) {
        final StringBuilder sb = new StringBuilder();
        string(value, sb);

        return LamaString.ofString(sb.toString());
    }

    public static void string(Object value, StringBuilder builder) {
        if (value == null) {
            return;
        }

        if (value instanceof Long v) {
            string((long) v, builder);
            return;
        }

        if (value instanceof LamaString v) {
            string(v, builder);
            return;
        }

        if (value instanceof LamaArray v) {
            string(v, builder);
            return;
        }

        if (value instanceof LamaSexp v) {
            string(v, builder);
            return;
        }

        if (value instanceof LamaFun v) {
            string(v, builder);
            return;
        }

        CompilerDirectives.transferToInterpreterAndInvalidate();
        throw new IllegalArgumentException("value of type " + value.getClass()
                + " is not a " + LamaLanguage.NAME + " value");
    }

    public static void string(long value, StringBuilder builder) {
        builder.append(value);
    }

    public static void string(LamaString value, StringBuilder builder) {
        builder.append('"').append(value).append('"');
    }

    public static void string(LamaArray value, StringBuilder builder) {
        if (value.values().length == 0) {
            builder.append("[]");
            return;
        }

        builder.append('[');
        string(value.values()[0], builder);

        for (int i = 1; i < value.values().length; ++i) {
            builder.append(", ");

            string(value.values()[i], builder);
        }

        builder.append(']');
    }

    public static void string(LamaSexp value, StringBuilder builder) {
        builder.append(value.tag());

        if (value.values().length == 0) {
            return;
        }

        builder.append(" (");
        string(value.values()[0], builder);

        for (int i = 1; i < value.values().length; ++i) {
            builder.append(", ");

            string(value.values()[i], builder);
        }

        builder.append(')');
    }

    public static void string(LamaFun value, StringBuilder builder) {
        builder.append("<closure 0x").append(Integer.toString(value.callTarget().hashCode(), 16));

        final MaterializedFrame frame = value.parentScope();
        final int fieldsNum = frame.getFrameDescriptor().getNumberOfSlots();
        for (int i = 0; i < fieldsNum; ++i) {
            builder.append(", ");

            string(frame.getValue(i), builder);
        }

        builder.append('>');
    }
}
