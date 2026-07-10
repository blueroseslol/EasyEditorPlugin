import * as UE from "ue";
export declare class GameplayRuntime {
    readonly gameInstance: UE.GameInstance;
    private started;
    constructor(gameInstance: UE.GameInstance);
    start(): void;
    stop(): void;
}
