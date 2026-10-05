import { expect, test } from 'claude-code/testing'
import { buildInput, modelDisplayName } from '../hooks/payload'

test('nom du modèle', () => {
  expect(modelDisplayName('claude-opus-4-8')).toBe('Claude Opus 4.8')
  expect(modelDisplayName('claude-sonnet-5-5[1m]')).toBe('Claude Sonnet 5.5')
  expect(modelDisplayName('autre')).toBe('autre')
})

test('JSON pour le script de la bague', () => {
  const input = buildInput({
    sessionId: 's1',
    model: 'claude-opus-4-8',
    effort: 'high',
    usage: {
      startedAt: 0,
      context: { window: 200000, percent: 42 },
      rateLimits: [{ kind: 'five_hour', percentUsed: 23 }],
    },
  })
  expect(input.effort).toEqual({ level: 'high' })
  expect(input.context_window.used_percentage).toBe(42)
  expect(input.rate_limits).toEqual({ five_hour: { used_percentage: 23 } })
})
