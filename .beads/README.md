# Beads state

Beads is the durable task and coordination store for this project.

The Dolt database is independent of Jujutsu history:

- Pull before selecting or modifying shared task state.
- Push immediately after claiming.
- Push after dependency, scope, ownership, review, or status changes.
- Note review bookmarks and pull requests.
- Publish a completed integration unit for review by default; merging still requires authorization.
- Close only after the implementation is integrated into `main`.

A Bead is not a commit. One Bead may contain several local jj commits and normally produces one
review bookmark or pull request.

Do not edit generated Beads storage directly. See
[the contributor workflow](../docs/contributor-workflow.md) for the supported commands.
