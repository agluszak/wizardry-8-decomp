import type { Plugin } from "@opencode-ai/plugin"

type PolicyEvent = Record<string, unknown>

async function policy(directory: string, event: PolicyEvent): Promise<Record<string, any>> {
  const proc = Bun.spawn(
    ["uv", "run", "--frozen", "python", "-m", "wiz8decomp.agent_hooks"],
    { cwd: directory, stdin: "pipe", stdout: "pipe", stderr: "pipe" },
  )
  const writer = proc.stdin
  writer.write(JSON.stringify({ cwd: directory, ...event }))
  writer.end()
  const output = await new Response(proc.stdout).text()
  await proc.exited
  if (!output.trim()) return {}
  return JSON.parse(output) as Record<string, any>
}

export const Wiz8Guard: Plugin = async (ctx) => {
  return {
    "experimental.chat.system.transform": async (input, output) => {
      const result = await policy(ctx.directory, {
        hook_event_name: "SessionStart",
        session_id: input.sessionID ?? "opencode",
        source: "resume",
      })
      const context = result.hookSpecificOutput?.additionalContext
      if (context) output.system.push(String(context))
    },
    "experimental.session.compacting": async (input, output) => {
      const result = await policy(ctx.directory, {
        hook_event_name: "SessionStart",
        session_id: input.sessionID,
        source: "compact",
      })
      const context = result.hookSpecificOutput?.additionalContext
      if (context) output.context.push(String(context))
    },
  }
}

export default Wiz8Guard
