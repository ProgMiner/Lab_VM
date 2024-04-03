package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import ru.byprogminer.lamagraalvm.nodes.builtin.StringBuiltin;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;
import ru.byprogminer.lamagraalvm.runtime.LamaString;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;

@RequiredArgsConstructor
@NodeInfo(shortName = "case", description = "case-of expression")
public class Case extends LamaExpr {

    private final int line, column;

    @Child @NonNull private LamaExpr value;

    @NonNull private final Matcher @NonNull[] branchMatchers;
    @Children @NonNull private LamaExpr @NonNull[] branchBodies;

    @Override
    public Object execute(VirtualFrame frame) {
        final Object value = this.value.execute(frame);

        for (int i = 0; i < branchMatchers.length; ++i) {
            if (branchMatchers[i].matches(frame, value)) {
                return branchBodies[i].execute(frame);
            }
        }

        CompilerDirectives.transferToInterpreterAndInvalidate();

        final StringBuilder valueStr = new StringBuilder();
        StringBuiltin.string(value, valueStr);

        throw exn(new IllegalArgumentException("match failure at " + line + ":" + column + ", value: " + valueStr));
    }

    public interface Matcher {

        boolean matches(Frame frame, Object value);

        static Matcher wildcard() {
            return (frame, value) -> true;
        }

        static Matcher sexp(String tag, List<? extends Matcher> matchers) {
            return (frame, value) -> {
                if (!(value instanceof LamaSexp sexp)) {
                    return false;
                }

                if (!Objects.equals(sexp.tag(), tag)) {
                    return false;
                }

                return matchArray(frame, sexp.values(), matchers);
            };
        }

        static Matcher array(List<? extends Matcher> matchers) {
            return (frame, value) -> {
                if (!(value instanceof LamaArray array)) {
                    return false;
                }

                return matchArray(frame, array.values(), matchers);
            };
        }

        static Matcher named(int slot, Matcher matcher) {
            return (frame, value) -> {
                if (!matcher.matches(frame, value)) {
                    return false;
                }

                // TODO special case for long
                frame.setObject(slot, value);
                return true;
            };
        }

        static Matcher decimal(long value) {
            return (frame, value1) -> value1 instanceof Long l && l == value;
        }

        static Matcher string(LamaString value) {
            return (frame, value1) -> value1 instanceof LamaString arr && Arrays.equals(arr.value(), value.value());
        }

        static Matcher boxShape() {
            return (frame, value) -> !(value instanceof Long);
        }

        static Matcher valShape() {
            return (frame, value) -> value instanceof Long;
        }

        static Matcher shape(Class<?> clazz) {
            return (frame, value) -> clazz.isInstance(value);
        }

        private static boolean matchArray(Frame frame, Object[] values, List<? extends Matcher> matchers) {
            if (values.length != matchers.size()) {
                return false;
            }

            for (int i = 0; i < values.length; ++i) {
                if (!matchers.get(i).matches(frame, values[i])) {
                    return false;
                }
            }

            return true;
        }
    }
}
