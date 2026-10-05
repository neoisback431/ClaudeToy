// Construit le JSON que lit tools/claudetoy_statusline.py (même forme que la status line de Claude Code).

import type { SessionUsage } from 'claude-code'

const MODEL_ID = /^claude-([a-z]+)-(\d+)-(\d+)(?:-\d{8})?(?:\[[^\]]*\])?$/

export function modelDisplayName(id: string): string {
  const match = MODEL_ID.exec(id)
  if (!match) return id
  const [, family = '', major, minor] = match
  return `Claude ${family.charAt(0).toUpperCase()}${family.slice(1)} ${major}.${minor}`
}

export function buildInput(facts: { sessionId: string; model: string; effort?: string; usage: SessionUsage }) {
  const { context, rateLimits } = facts.usage
  return {
    session_id: facts.sessionId,
    model: { id: facts.model, display_name: modelDisplayName(facts.model) },
    effort: facts.effort ? { level: facts.effort } : undefined,
    context_window: { used_percentage: context.percent ?? null },
    rate_limits: Object.fromEntries(rateLimits.map((l) => [l.kind, { used_percentage: l.percentUsed }])),
  }
}
