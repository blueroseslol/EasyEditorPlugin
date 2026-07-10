import * as UE from "ue";

export class GameplayRuntime {
    private started = false;

    public constructor(public readonly gameInstance: UE.GameInstance) {}

    public start(): void {
        if (this.started) {
            return;
        }
        this.started = true;
    }

    public stop(): void {
        if (!this.started) {
            return;
        }
        this.started = false;
    }
}
