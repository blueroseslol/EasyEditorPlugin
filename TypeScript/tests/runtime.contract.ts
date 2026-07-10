import * as UE from "ue";
import { GameplayRuntime } from "../src";

declare const gameInstance: UE.GameInstance;

const runtime = new GameplayRuntime(gameInstance);
runtime.start();
runtime.stop();
