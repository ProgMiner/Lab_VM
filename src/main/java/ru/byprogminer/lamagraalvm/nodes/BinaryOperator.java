package ru.byprogminer.lamagraalvm.nodes;

import com.oracle.truffle.api.dsl.GenerateNodeFactory;
import com.oracle.truffle.api.dsl.NodeChild;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.nodes.NodeInfo;

@NodeChild(value = "lhs", type = LamaExpr.class)
@NodeChild(value = "rhs", type = LamaExpr.class)
public abstract class BinaryOperator extends LamaExpr {

    @GenerateNodeFactory
    @NodeInfo(shortName = "*", description = "binary multiply operator")
    public static abstract class Multiply extends BinaryOperator {

        @Specialization
        public long multiply(long lhs, long rhs) {
            return lhs * rhs;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "/", description = "binary divide operator")
    public static abstract class Divide extends BinaryOperator {

        @Specialization
        public long divide(long lhs, long rhs) {
            return lhs / rhs;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "%", description = "binary remainder operator")
    public static abstract class Remainder extends BinaryOperator {

        @Specialization
        public long remainder(long lhs, long rhs) {
            return lhs % rhs;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "+", description = "binary plus operator")
    public static abstract class Plus extends BinaryOperator {

        @Specialization
        public long plus(long lhs, long rhs) {
            return lhs + rhs;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "-", description = "binary minus operator")
    public static abstract class Minus extends BinaryOperator {

        @Specialization
        public long minus(long lhs, long rhs) {
            return lhs - rhs;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "==", description = "binary equals operator")
    public static abstract class Equals extends BinaryOperator {

        @Specialization
        public long equals(Object lhs, Object rhs) {
            return lhs == rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "!=", description = "binary not equals operator")
    public static abstract class NotEquals extends BinaryOperator {

        @Specialization
        public long notEquals(Object lhs, Object rhs) {
            return lhs != rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "<", description = "binary less operator")
    public static abstract class Less extends BinaryOperator {

        @Specialization
        public long less(long lhs, long rhs) {
            return lhs < rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "<=", description = "binary less or equal operator")
    public static abstract class LessOrEqual extends BinaryOperator {

        @Specialization
        public long lessOrEqual(long lhs, long rhs) {
            return lhs <= rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = ">", description = "binary greater operator")
    public static abstract class Greater extends BinaryOperator {

        @Specialization
        public long greater(long lhs, long rhs) {
            return lhs > rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = ">=", description = "binary greater or equal operator")
    public static abstract class GreaterOrEqual extends BinaryOperator {

        @Specialization
        public long greaterOrEqual(long lhs, long rhs) {
            return lhs >= rhs ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "&&", description = "binary logical and operator")
    public static abstract class And extends BinaryOperator {

        @Specialization
        public long and(long lhs, long rhs) {
            return lhs != 0 && rhs != 0 ? 1 : 0;
        }
    }

    @GenerateNodeFactory
    @NodeInfo(shortName = "!!", description = "binary logical or operator")
    public static abstract class Or extends BinaryOperator {

        @Specialization
        public long or(long lhs, long rhs) {
            return lhs != 0 || rhs != 0 ? 1 : 0;
        }
    }
}
