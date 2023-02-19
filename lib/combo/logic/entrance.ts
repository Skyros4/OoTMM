import { Random, sample } from "../random";
import { Settings } from "../settings";
import { DUNGEONS_REGIONS, ExprMap, World, WorldEntrance } from "./world";
import { Pathfinder, EntranceOverrides } from './pathfind';
import { Monitor } from "../monitor";

export type EntranceShuffleResult = {
  overrides: {[k: string]: {[k:string]: { from: string, to: string }}};
  boss: number[];
  dungeons: number[];
};

const BOSS_INDEX_BY_DUNGEON = {
  DT: 0,
  DC: 1,
  JJ: 2,
  Forest: 3,
  Fire: 4,
  Water: 5,
  Shadow: 6,
  Spirit: 7,
  WF: 8,
  SH: 9,
  GB: 10,
  IST: 11,
} as {[k: string]: number};

const DUNGEON_INDEX = {
  DT: 0,
  DC: 1,
  JJ: 2,
  Forest: 3,
  Fire: 4,
  Water: 5,
  Shadow: 6,
  Spirit: 7,
  WF: 8,
  SH: 9,
  GB: 10,
  IST: 11,
  ST: 12,
};

export class LogicPassEntrances {
  private pathfinder: Pathfinder;

  constructor(
    private readonly input: {
      world: World;
      settings: Settings;
      random: Random;
      monitor: Monitor;
    },
  ) {
    this.pathfinder = new Pathfinder(input.world, input.settings);
  }
  private result: EntranceShuffleResult = {
    overrides: {},
    boss: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
    dungeons: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
  };

  private isBossAssignable(src: WorldEntrance, dst: WorldEntrance, overrides: EntranceOverrides) {
    if (this.input.settings.erBoss === 'ownGame') {
      /* Check that the entrances are from the same game */
      /* TODO: Ugly */
      const prefixA = src.from.split(' ')[0];
      const prefixB = dst.from.split(' ')[0];
      if (prefixA !== prefixB) {
        return false;
      }
    }

    /* Pathfind with the override */
    overrides = { ...overrides, [src.from]: { [src.to]: dst.to } };
    const pathfinderState = this.pathfinder.run(null, { recursive: true, entranceOverrides: overrides, ignoreItems: true });

    /* Check if every location is reachable */
    const dungeon = this.input.world.areas[dst.to].dungeon!;
    const locations = [...this.input.world.dungeons[dungeon]];
    if (locations.some(l => !pathfinderState.locations.has(l))) {
      return false;
    }
    return true;
  }

  private fixBosses() {
    const bossEntrances = this.input.world.entrances.filter(e => this.input.world.areas[e.to].boss);
    const bossEntrancesByDungeon: {[k: string]: WorldEntrance} = {};
    const overrides: EntranceOverrides = {};
    const combinations: {[k: string]: string[]} = {};
    const placed: {[k: string]: string} = {};

    /* Set up null overrides */
    for (const e of bossEntrances) {
      const override = overrides[e.from] || {};
      override[e.to] = null;
      overrides[e.from] = override;
      const dungeon = this.input.world.areas[e.to].dungeon!;
      bossEntrancesByDungeon[dungeon] = e;
    }

    /* Extract dungeons and events */
    const dungeons = Object.keys(bossEntrancesByDungeon);
    const events: {[k: string]: ExprMap} = {};
    const lastAreaByDungeon: {[k: string]: string} = {};
    const dungeonsBossAreas: {[k: string]: string[]} = {};
    for (const d of dungeons) {
      events[d] = {};
      dungeonsBossAreas[d] = [];
      for (const a in this.input.world.areas) {
        const area = this.input.world.areas[a];
        if (area.dungeon === d && area.boss) {
          events[d] = { ...events[d], ...area.events };
          area.events = {};
          lastAreaByDungeon[d] = a;
          dungeonsBossAreas[d].push(a);
        }
      }
    }

    /* Set up base reachability */
    for (const dungeonSrc of dungeons) {
      for (const dungeonDst of dungeons) {
        const src = bossEntrancesByDungeon[dungeonSrc];
        const dst = bossEntrancesByDungeon[dungeonDst];
        if (this.isBossAssignable(src, dst, overrides)) {
          const combination = combinations[dungeonDst] || [];
          combination.push(dungeonSrc);
          combinations[dungeonDst] = combination;
        }
      }
    }

    for (;;) {
      const keys = Object.keys(combinations);
      if (keys.length === 0) {
        break;
      }
      const sorted = keys.sort((a, b) => combinations[a].length - combinations[b].length);
      const boss = sorted[0];
      const src = sample(this.input.random, combinations[boss]);
      placed[src] = boss;
      delete combinations[boss];
      for (const k of Object.keys(combinations)) {
        combinations[k] = combinations[k].filter(s => s !== src);
      }
    }

    /* Alter the world */
    for (const srcDungeon in placed) {
      const dstDungeon = placed[srcDungeon];
      const src = bossEntrancesByDungeon[srcDungeon];
      const dst = bossEntrancesByDungeon[dstDungeon];

      /* Mark the blue warp */
      this.result.boss[BOSS_INDEX_BY_DUNGEON[dstDungeon]] = BOSS_INDEX_BY_DUNGEON[srcDungeon];

      /* Replace the entrance */
      const srcArea = this.input.world.areas[src.from];
      const expr = srcArea.exits[src.to];
      delete srcArea.exits[src.to];
      srcArea.exits[dst.to] = expr;

      /* Replace the dungeon tag */
      for (const a of dungeonsBossAreas[dstDungeon]) {
        const area = this.input.world.areas[a];
        area.dungeon = srcDungeon;

        for (const l in area.locations) {
          this.input.world.regions[l] = DUNGEONS_REGIONS[srcDungeon];
          this.input.world.dungeons[srcDungeon].add(l);
          this.input.world.dungeons[dstDungeon].delete(l);
        }
      }

      /* Replace the events */
      const dstBoss = this.input.world.areas[lastAreaByDungeon[dstDungeon]];
      dstBoss.events = events[srcDungeon];

      /* Mark the override */
      const override = this.result.overrides[src.from] || {};
      override[src.to] = { from: dst.from, to: dst.to };
      this.result.overrides[src.from] = override;
    }
  }

  private fixDungeons() {
    const dungeonEntrances = this.input.world.entrances.filter(e => { console.log(e); return !this.input.world.areas[e.from].dungeon && this.input.world.areas[e.to].dungeon });
    console.log(dungeonEntrances);
  }

  run() {
    this.input.monitor.log('Logic: Entrances');

    if (this.input.settings.erDungeons !== 'none') {
      this.fixDungeons();
      process.exit(0);
    }

    if (this.input.settings.erBoss !== 'none') {
      this.fixBosses();
    }

    return { entrances: this.result };
  }
};

