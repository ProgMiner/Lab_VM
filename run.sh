
jar=target/lama-graalvm-1.0-SNAPSHOT.jar
main=ru.byprogminer.lamagraalvm.Main

"$JAVA_HOME/bin/java" -cp "$jar" "-Dtruffle.class.path.append=$jar" "$main"
