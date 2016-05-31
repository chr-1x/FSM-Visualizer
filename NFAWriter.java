/* NFAWriter.java
 * Most of this file (c) the authors of the UW CSE 311 Grep Project, 
 * primarily Adam Blank. Use falls entirely under his discretion!
 * 
 * Packaging into NFA writer java class by Andrew Chronister
 */

import java.io.PrintWriter;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;
import java.lang.reflect.Field;

public class NFAWriter
{
    public static String write(NFA n) {
        Set<FSMState> states = (Set<FSMState>) getField(n, "states");
        FSMState startState = (FSMState) getField(n, "startState");
        Set<FSMState> finalStates = (Set<FSMState>) getField(n, "finalStates");
        Set<FSMTransition> transitions = (Set<FSMTransition>) getField(n, "transitions");
        Map<FSMState, Set<FSMTransition>> graph = NFAConstructionTester.makeAdjacencyList(states, transitions);
        StringBuffer out = new StringBuffer();

        // Give each state a human-readable name
        Map<FSMState, String> niceNames = new HashMap<>();

        int finalStateCounter = 0;
        int stateCounter = 0;
        for (FSMState state : states) {
            if (finalStates.contains(state)) {
                niceNames.put(state, "ACCEPT" + finalStateCounter);
                finalStateCounter += 1;
            } else {
                niceNames.put(state, "STATE" + stateCounter);
                stateCounter += 1;
            }
        }

        // Override existing state name
        niceNames.put(startState, "START");

        // Print out general info about states
        out.append("Nfa (id:" + n.hashCode() + ")\n");
        out.append("    All States (with hashcodes): \n" + String.join("",
                niceNames.entrySet().stream()
                    .map(e -> String.format("        %s (obj id: %d)\n", 
                        e.getValue(), 
                        e.getKey().hashCode()))
                    .sorted()
                    .collect(Collectors.toList())));
        out.append("    Start State:   " + niceNames.get(startState) + "\n");
        out.append("    Accept States: " + 
                niceNames.entrySet().stream()
                    .filter(e -> finalStates.contains(e.getKey()))
                    .map(e -> e.getValue())
                    .sorted()
                    .collect(Collectors.toList()) + "\n");

        // Print out all transitions
        out.append("    Transitions:" + "\n");
        for (FSMState state : graph.keySet()) {
            if (graph.get(state).isEmpty()) {
                continue;
            }
            out.append("        " + niceNames.get(state) + ":\n");
            for (FSMTransition arrow : graph.get(state)) {
                // Bracket the character in unusual bytes so that the parser can
                // read it. Normal quote marks won't work here unless quotes
                // appearing in the transition are escaped.
                out.append(String.format("            \001%s\001 -> %s\n", 
                            arrow.getCharacter(),
                            niceNames.get(arrow.getDestination())));
            }
        }
        return out.toString();
    }

    public static void writeToFile(NFA n, String filename)
    {
        if (filename == "") {
            filename += n.hashCode() + ".nfa";
        }

        try {
            PrintWriter writer = new PrintWriter(filename);
            writer.print(write(n));
            writer.close();
        }
        catch (java.io.FileNotFoundException e)
        {
            return;
        }
    }

    private static Object getField(NFA nfa, String name) {
        try {
            Field field = nfa.getClass().getDeclaredField(name);
            field.setAccessible(true);
            return field.get(nfa);
        } catch (NoSuchFieldException ex) {
            throw new RuntimeException(String.format(
                        "NFA class is missing the \"%s\" field.", name), ex);
        } catch (IllegalAccessException ex) {
            throw new RuntimeException(String.format(
                        "Unexpected access error with \"%s\" field.", name), ex);
        }
    }
}
