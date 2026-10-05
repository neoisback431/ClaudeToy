// claudetoy-statusline : alimente la bague ClaudeToy depuis Claude Code (terminal ou Desktop),
// sans affichage. Les données viennent de l'API des mods ($.session.*) et sont passées sur stdin
// à claudetoy_statusline.py (livré dans le plugin), qui les envoie sur le port série.

import type { EngineInterface, PluginOptions, Register } from 'claude-code'
import { buildInput } from './payload'

const RUN_TIMEOUT_MS = 5_000

// Valeurs d'un module qui se recharge : le module repart de zéro, c'est voulu.
let effort: string | undefined
let running = false

async function push($: EngineInterface, options: PluginOptions) {
  if (running) return
  running = true
  try {
    const input = buildInput({
      sessionId: await $.session.id(),
      model: await $.session.model(),
      effort,
      usage: await $.session.usage(),
    })
    const script = `${$.plugin.root}/claudetoy_statusline.py`
    await $.process.run([String(options.python || 'python'), script], {
      stdin: JSON.stringify(input),
      timeoutMs: RUN_TIMEOUT_MS,
    })
  } catch (error) {
    $.ui.log(`claudetoy-statusline: ${error instanceof Error ? error.message : String(error)}`, { to: 'debug' })
  } finally {
    running = false
  }
}

export const register: Register = (on, options) => {
  const refreshMs = Math.max(1, Number(options.refreshSeconds) || 5) * 1000

  on('session.start', async ($, e, next) => {
    const result = await next(e)
    await push($, options)
    $.clock.every(refreshMs, () => push($, options))
    return result
  })

  // L'effort n'est pas dans $.session : on le lit sur chaque requête envoyée au modèle.
  on('turn.step', async function* ($, e, next) {
    effort = e.effort === undefined ? undefined : String(e.effort)
    return yield* next(e)
  })

  on('turn.complete', async ($, e, next) => {
    const result = await next(e)
    await push($, options)
    return result
  })
}
