#!/usr/bin/env python3
import os
import json
import subprocess
import sys

def run_command(cmd, cwd=None):
    """Run a shell command and return stdout or None on error."""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            return result.stdout.strip()
        else:
            return None
    except Exception as e:
        print(f"Error running {cmd}: {e}", file=sys.stderr)
        return None

def get_repo_name(repo_path):
    """Get repo name from git remote origin or dirname."""
    origin = run_command("git config --get remote.origin.url", cwd=repo_path)
    if origin and "github.com" in origin:
        return os.path.basename(origin).replace(".git", "")
    return os.path.basename(repo_path)

def get_description(repo_path):
    """Get description from README.md or git config."""
    readme_path = os.path.join(repo_path, "README.md")
    if os.path.exists(readme_path):
        with open(readme_path, "r") as f:
            lines = f.readlines()
            for line in lines:
                if line.startswith("# "):
                    return line[2:].strip()
    desc = run_command("git config --get core.description", cwd=repo_path)
    return desc or ""

def get_github_url(repo_path):
    """Get GitHub public URL if it's a GitHub repo."""
    origin = run_command("git config --get remote.origin.url", cwd=repo_path)
    if origin and "github.com" in origin:
        owner_repo = origin.split("github.com/")[-1].replace(".git", "").replace(".git/", "")
        gh_output = run_command(f"gh repo view {owner_repo} --json htmlUrl")
        if gh_output:
            try:
                data = json.loads(gh_output)
                return data.get("htmlUrl")
            except json.JSONDecodeError:
                pass
    return None

def scan_projects(scan_dir):
    """Scan directory for git repos and extract metadata."""
    projects = []
    for item in os.listdir(scan_dir):
        repo_path = os.path.join(scan_dir, item)
        if os.path.isdir(repo_path) and os.path.exists(os.path.join(repo_path, ".git")):
            name = get_repo_name(repo_path)
            description = get_description(repo_path)
            github_url = get_github_url(repo_path)
            project = {
                "name": name,
                "path": repo_path,
                "description": description,
                "github_url": github_url
            }
            projects.append(project)
    return projects

def generate_summary(projects, output_file):
    """Generate a Markdown summary."""
    with open(output_file, "w") as f:
        f.write("# Project Summary\n\n")
        f.write(f"Scanned {len(projects)} projects.\n\n")
        f.write("| Name | Path | Description | GitHub URL |\n")
        f.write("|------|------|-------------|------------|\n")
        for p in projects:
            url = p["github_url"] or "N/A"
            f.write(f"| {p['name']} | {p['path']} | {p['description']} | {url} |\n")

if __name__ == "__main__":
    scan_dir = os.path.expanduser("~/IdeaProjects")
    json_file = os.path.expanduser("~/dev/projects_metadata.json")
    summary_file = os.path.expanduser("~/dev/projects_summary.md")

    if not os.path.exists(scan_dir):
        print(f"Scan directory {scan_dir} does not exist.")
        sys.exit(1)

    projects = scan_projects(scan_dir)
    with open(json_file, "w") as f:
        json.dump(projects, f, indent=2)

    generate_summary(projects, summary_file)
    print(f"Metadata saved to {json_file}")
    print(f"Summary saved to {summary_file}")