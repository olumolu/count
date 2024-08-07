use std::fs;
use std::io;
use std::path::Path;
use std::process::Command;
use regex::Regex;

struct Language {
    name: String,
    file_ext: String,
    single_comment_pattern: Option<String>,
    block_comment_start_pattern: Option<String>,
    block_comment_end_pattern: Option<String>,
}

fn get_language(file_ext: &str) -> Option<Language> {
    match file_ext {
        "c" => Some(Language {
            name: "C".to_string(),
            file_ext: ".c".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "cpp" => Some(Language {
            name: "C++".to_string(),
            file_ext: ".cpp".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "rs" => Some(Language {
            name: "Rust".to_string(),
            file_ext: ".rs".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "asm" => Some(Language {
            name: "Assembly".to_string(),
            file_ext: ".asm".to_string(),
            single_comment_pattern: Some("^\\s*;".to_string()),
            block_comment_start_pattern: None,
            block_comment_end_pattern: None,
        }),
        "java" => Some(Language {
            name: "Java".to_string(),
            file_ext: ".java".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "py" => Some(Language {
            name: "Python".to_string(),
            file_ext: ".py".to_string(),
            single_comment_pattern: Some("^\\s*#".to_string()),
            block_comment_start_pattern: None,
            block_comment_end_pattern: None,
        }),
        "js" => Some(Language {
            name: "JavaScript".to_string(),
            file_ext: ".js".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "php" => Some(Language {
            name: "PHP".to_string(),
            file_ext: ".php".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "rb" => Some(Language {
            name: "Ruby".to_string(),
            file_ext: ".rb".to_string(),
            single_comment_pattern: Some("^\\s*#".to_string()),
            block_comment_start_pattern: None,
            block_comment_end_pattern: None,
        }),
        "swift" => Some(Language {
            name: "Swift".to_string(),
            file_ext: ".swift".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "go" => Some(Language {
            name: "Go".to_string(),
            file_ext: ".go".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "dart" => Some(Language {
            name: "Dart".to_string(),
            file_ext: ".dart".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "kt" => Some(Language {
            name: "Kotlin".to_string(),
            file_ext: ".kt".to_string(),
            single_comment_pattern: Some("^\\s*//".to_string()),
            block_comment_start_pattern: Some("^\\s*/\\*".to_string()),
            block_comment_end_pattern: Some("\\*/".to_string()),
        }),
        "pl" => Some(Language {
            name: "Perl".to_string(),
            file_ext: ".pl".to_string(),
            single_comment_pattern: Some("^\\s*#".to_string()),
            block_comment_start_pattern: None,
            block_comment_end_pattern: None,
        }),
        "erl" => Some(Language {
            name: "Erlang".to_string(),
            file_ext: ".erl".to_string(),
            single_comment_pattern: Some("^\\s*%".to_string()),
            block_comment_start_pattern: None,
            block_comment_end_pattern: None,
        }),
        _ => None,
    }
}

fn count_lines(file_path: &str, with_comments: &mut usize, without_comments: &mut usize) -> io::Result<()> {
    let file = fs::File::open(file_path)?;
    let reader = io::BufReader::new(file);

    let ext = Path::new(file_path).extension().and_then(|s| s.to_str());
    let lang = match ext {
        Some(ext) => get_language(ext),
        None => return Ok(()),
    };

    let lang = match lang {
        Some(lang) => lang,
        None => return Ok(()),
    };

    let mut in_block_comment = false;

    let single_comment_pattern = lang.single_comment_pattern.as_ref().map(|s| Regex::new(s).unwrap());
    let block_comment_start_pattern = lang.block_comment_start_pattern.as_ref().map(|s| Regex::new(s).unwrap());
    let block_comment_end_pattern = lang.block_comment_end_pattern.as_ref().map(|s| Regex::new(s).unwrap());

    for line in reader.lines() {
        let line = line?;
        *with_comments += 1;

        if in_block_comment {
            if let Some(pattern) = &block_comment_end_pattern {
                if pattern.is_match(&line) {
                    in_block_comment = false;
                }
            }
            continue;
        }

        if let Some(pattern) = &block_comment_start_pattern {
            if pattern.is_match(&line) {
                in_block_comment = true;
                continue;
            }
        }

        if let Some(pattern) = &single_comment_pattern {
            if pattern.is_match(&line) {
                continue;
            }
        }

        let stripped_line = line.trim();
        if !stripped_line.is_empty() {
            *without_comments += 1;
        }
    }

    Ok(())
}

fn traverse_repo(repo_path: &str, with_comments: &mut usize, without_comments: &mut usize) -> io::Result<()> {
    for entry in fs::read_dir(repo_path)? {
        let entry = entry?;
        let path = entry.path();

        if path.is_dir() {
            traverse_repo(path.to_str().unwrap(), with_comments, without_comments)?;
        } else if path.is_file() {
            count_lines(path.to_str().unwrap(), with_comments, without_comments)?;
        }
    }

    Ok(())
}

fn clone_repo(repo_url: &str, dest_dir: &str) -> io::Result<()> {
    let output = Command::new("git")
        .arg("clone")
        .arg(repo_url)
        .arg(dest_dir)
        .output()?;

    if !output.status.success() {
        return Err(io::Error::new(io::ErrorKind::Other, "Failed to clone repository"));
    }

    Ok(())
}

fn main() -> io::Result<()> {
    let mut input = String::new();
    println!("Enter the repository URL or directory path:");
    io::stdin().read_line(&mut input)?;

    let input = input.trim();

    if input.starts_with("http://") || input.starts_with("https://") || input.starts_with("git@") {
        let temp_dir = "/tmp/repo_clone";
        fs::create_dir_all(temp_dir)?;

        clone_repo(input, temp_dir)?;

        let mut with_comments = 0;
        let mut without_comments = 0;
        traverse_repo(temp_dir, &mut with_comments, &mut without_comments)?;

        println!("Lines of code with comments: {}", with_comments);
        println!("Lines of code without comments: {}", without_comments);

        fs::remove_dir_all(temp_dir)?;
    } else {
        let mut with_comments = 0;
        let mut without_comments = 0;
        traverse_repo(input, &mut with_comments, &mut without_comments)?;

        println!("Lines of code with comments: {}", with_comments);
        println!("Lines of code without comments: {}", without_comments);
    }

    Ok(())
}
