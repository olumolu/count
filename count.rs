use std::fs;
use std::io;
use std::path::Path;
use std::process::Command;
use regex::Regex;

const MAX_PATH_LENGTH: usize = 1024;
const MAX_REPO_URL_LENGTH: usize = 256;

fn clone_repo(repo_url: &str, dest_dir: &str) -> io::Result<()> {
    let command = format!("git clone {} {}", repo_url, dest_dir);
    let output = Command::new("sh")
        .arg("-c")
        .arg(command)
        .output()?;
    if !output.status.success() {
        return Err(io::Error::new(io::ErrorKind::Other, "Failed to clone repository"));
    }
    Ok(())
}

fn count_lines(file_path: &str, with_comments: &mut usize, without_comments: &mut usize) -> io::Result<()> {
    let file = fs::File::open(file_path)?;
    let reader = io::BufReader::new(file);
    let mut in_block_comment = false;
    let single_comment_pattern = Regex::new(r"^//").unwrap();
    let block_comment_start_pattern = Regex::new(r"/\*").unwrap();
    let block_comment_end_pattern = Regex::new(r"\*/").unwrap();
    for line in reader.lines() {
        let line = line?;
        *with_comments += 1;
        if in_block_comment {
            if block_comment_end_pattern.is_match(&line) {
                in_block_comment = false;
            }
            continue;
        }
        if block_comment_start_pattern.is_match(&line) {
            in_block_comment = true;
            continue;
        }
        if single_comment_pattern.is_match(&line) {
            continue;
        }
        let stripped_line = line.trim();
        if !stripped_line.is_empty() {
            *without_comments += 1;
        }
    }
    Ok(())
}

fn traverse_repo(repo_path: &str, total_with_comments: &mut usize, total_without_comments: &mut usize) -> io::Result<()> {
    for entry in fs::read_dir(repo_path)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_dir() {
            traverse_repo(path.to_str().unwrap(), total_with_comments, total_without_comments)?;
        } else if path.is_file() {
            let file_name = path.file_name().unwrap().to_str().unwrap();
            let file_ext = file_name.split('.').last().unwrap().to_lowercase();
            match file_ext.as_str() {
                "c" | "cpp" | "cc" | "cxx" | "c++" | "h" | "hpp" | "hxx" | "h++" |
                "java" | "js" | "py" | "php" | "rb" | "swift" | "go" | "rs" |
                "scala" | "kt" | "groovy" | "erl" | "elixir" | "lua" | "perl" |
                "pl" | "pm" | "t" | "pod" | "pas" | "p" | "d" | "f" | "f90" |
                "f95" | "f03" | "f08" | "f11" | "f18" | "f77" | "f66" | "f60" |
                "f50" | "f40" | "f30" | "f20" | "f10" | "f00" | "f" => {
                    count_lines(path.to_str().unwrap(), total_with_comments, total_without_comments)?;
                }
                _ => {}
            }
        }
    }
    Ok(())
}

fn main() -> io::Result<()> {
    println!("Enter the repository URL:");
    let mut repo_url = String::new();
    io::stdin().read_line(&mut repo_url)?;
    let repo_url = repo_url.trim();
    let temp_dir = "/tmp/repo_clone";
    fs::create_dir_all(temp_dir)?;
    clone_repo(repo_url, temp_dir)?;
    let mut total_with_comments = 0;
    let mut total_without_comments = 0;
    traverse_repo(temp_dir, &mut total_with_comments, &mut total_without_comments)?;
    println!("Lines of code with comments: {}", total_with_comments);
    println!("Lines of code without comments: {}", total_without_comments);
    fs::remove_dir_all(temp_dir)?;
    Ok(())
}
