import { formatGreeting } from "./Greeting";

/** Example entry point: no editor APIs and no startup side effects. */
export function pluginGreeting(name: string): string {
    return formatGreeting(name);
}
