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

function toolName(tool: string): string {
  if (tool === "bash" || tool === "shell") return "Bash"
  if (tool === "apply_patch" || tool === "edit" || tool === "write") return "apply_patch"
  return tool
}

export const Wiz8Guard: Plugin = async (ctx) => {
  return {
    "tool.execute.before": async (input, output) => {
      const result = await policy(ctx.directory, {
        hook_event_name: "PreToolUse",
        session_id: input.sessionID,
        tool_name: toolName(input.tool),
        tool_use_id: input.callID,
        tool_input: output.args,
      })
      const specific = result.hookSpecificOutput as Record<string, any> | undefined
      if (specific?.permissionDecision === "deny") {
        throw new Error(String(specific.permissionDecisionReason ?? "Blocked by repository policy."))
      }
    },
    "tool.execute.after": async (input, output) => {
      const result = await policy(ctx.directory, {
        hook_event_name: "PostToolUse",
        session_id: input.sessionID,
        tool_name: toolName(input.tool),
        tool_use_id: input.callID,
        tool_input: output.args,
        tool_response: output.output,
      })
      if (result.continue === false) {
        output.output = `${String(result.reason ?? "Tool output was bounded.")}\n${String(result.hookSpecificOutput?.additionalContext ?? "")}`
      }
    },
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
