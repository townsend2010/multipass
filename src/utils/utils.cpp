/*
 * Copyright (C) 2017-2018 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <multipass/logging/log.h>
#include <multipass/ssh/ssh_session.h>
#include <multipass/utils.h>
#include <multipass/virtual_machine.h>

#include <fmt/format.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUuid>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <random>
#include <regex>
#include <sstream>

namespace mp = multipass;
namespace mpl = multipass::logging;

namespace
{
const std::regex bytes_regex{"^[[:digit:]]+((B)?)$", std::regex::optimize};
const std::regex kilobytes_regex{"^[[:digit:]]+((K)(B)?)$", std::regex::optimize};
const std::regex megabytes_regex{"^[[:digit:]]+((M)(B)?)$", std::regex::optimize};
const std::regex gigabytes_regex{"^[[:digit:]]+((G)(B)?)$", std::regex::optimize};

auto quote_for(const std::string& arg, mp::utils::QuoteType quote_type)
{
    if (quote_type == mp::utils::QuoteType::no_quotes)
        return "";
    return arg.find('\'') == std::string::npos ? "'" : "\"";
}
} // namespace

QDir mp::utils::base_dir(const QString& path)
{
    QFileInfo info{path};
    return info.absoluteDir();
}

bool mp::utils::valid_memory_value(const QString& mem_string)
{
    QRegExp matcher("\\d+((K|M|G)(B){0,1}){0,1}$");

    return matcher.exactMatch(mem_string);
}

bool mp::utils::valid_hostname(const QString& name_string)
{
    QRegExp matcher("^([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9])");

    return matcher.exactMatch(name_string);
}

bool mp::utils::invalid_target_path(const QString& target_path)
{
    QString sanitized_path{QDir::cleanPath(target_path)};
    QRegExp matcher("/+|/+(dev|proc|sys)(/.*)*|/+home(/*)(/ubuntu/*)*");

    return matcher.exactMatch(sanitized_path);
}

int mp::utils::to_megabytes(const std::string& mem_value)
{
    if (std::regex_match(mem_value, bytes_regex))
    {
        return std::stoi(mem_value) / 1048576;
    }
    else if (std::regex_match(mem_value, kilobytes_regex))
    {
        return std::stoi(mem_value) / 1024;
    }
    else if (std::regex_match(mem_value, megabytes_regex))
    {
        return std::stoi(mem_value);
    }
    else if (std::regex_match(mem_value, gigabytes_regex))
    {
        return std::stoi(mem_value) * 1024;
    }

    return -1;
}

int mp::utils::to_gigabytes(const std::string& mem_value)
{
    if (std::regex_match(mem_value, bytes_regex))
    {
        return std::stoi(mem_value) / 1073741824;
    }
    else if (std::regex_match(mem_value, kilobytes_regex))
    {
        return std::stoi(mem_value) / 1048576;
    }
    else if (std::regex_match(mem_value, megabytes_regex))
    {
        return std::stoi(mem_value) / 1024;
    }
    else if (std::regex_match(mem_value, gigabytes_regex))
    {
        return std::stoi(mem_value);
    }

    return -1;
}

std::string mp::utils::to_cmd(const std::vector<std::string>& args, QuoteType quote_type)
{
    fmt::memory_buffer buf;
    for (auto const& arg : args)
    {
        fmt::format_to(buf, "{0}{1}{0} ", quote_for(arg, quote_type), arg);
    }

    // Remove the last space inserted
    auto cmd = fmt::to_string(buf);
    cmd.pop_back();
    return cmd;
}

bool mp::utils::run_cmd_for_status(const QString& cmd, const QStringList& args)
{
    QProcess proc;
    proc.setProgram(cmd);
    proc.setArguments(args);

    proc.start();
    proc.waitForFinished();

    return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
}

std::string mp::utils::run_cmd_for_output(const QString& cmd, const QStringList& args)
{
    QProcess proc;
    proc.setProgram(cmd);
    proc.setArguments(args);

    proc.start();
    proc.waitForFinished();

    return proc.readAllStandardOutput().trimmed().toStdString();
}

std::string& mp::utils::trim_end(std::string& s)
{
    auto rev_it = std::find_if(s.rbegin(), s.rend(), [](char ch) { return !std::isspace(ch); });
    s.erase(rev_it.base(), s.end());
    return s;
}

std::string mp::utils::escape_char(const std::string& in, char c)
{
    return std::regex_replace(in, std::regex({c}), fmt::format("\\{}", c));
}

std::vector<std::string> mp::utils::split(const std::string& string, const std::string& delimiter)
{
    std::regex regex(delimiter);
    return {std::sregex_token_iterator{string.begin(), string.end(), regex, -1}, std::sregex_token_iterator{}};
}

std::string mp::utils::generate_mac_address()
{
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist{0, 255};

    gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::array<int, 3> octets{{dist(gen), dist(gen), dist(gen)}};
    return fmt::format("52:54:00:{:02x}:{:02x}:{:02x}", octets[0], octets[1], octets[2]);
}

void mp::utils::wait_until_ssh_up(VirtualMachine* virtual_machine, std::chrono::milliseconds timeout,
                                  std::function<void()> const& process_vm_events)
{
    auto action = [virtual_machine, &process_vm_events] {
        process_vm_events();
        try
        {
            mp::SSHSession session{virtual_machine->ssh_hostname(), virtual_machine->ssh_port()};
            virtual_machine->state = VirtualMachine::State::running;
            virtual_machine->update_state();
            return mp::utils::TimeoutAction::done;
        }
        catch (const std::exception&)
        {
            return mp::utils::TimeoutAction::retry;
        }
    };
    auto on_timeout = [virtual_machine] {
        virtual_machine->state = VirtualMachine::State::unknown;
        virtual_machine->update_state();
        return std::runtime_error("timed out waiting for ssh service to start");
    };

    mp::utils::try_action_for(on_timeout, timeout, action);
}

void mp::utils::wait_for_cloud_init(mp::VirtualMachine* virtual_machine, std::chrono::milliseconds timeout,
                                    std::function<void()> const& process_vm_events)
{
    auto action = [virtual_machine, &process_vm_events] {
        process_vm_events();
        try
        {
            mp::SSHSession session{virtual_machine->ssh_hostname(), virtual_machine->ssh_port(),
                                   virtual_machine->ssh_username(), virtual_machine->key_provider};
            auto ssh_process = session.exec({"[ -e /var/lib/cloud/instance/boot-finished ]"});
            return ssh_process.exit_code() == 0 ? mp::utils::TimeoutAction::done : mp::utils::TimeoutAction::retry;
        }
        catch (const std::exception& e)
        {
            mpl::log(mpl::Level::warning, virtual_machine->vm_name, e.what());
            return mp::utils::TimeoutAction::retry;
        }
    };
    auto on_timeout = [] { return std::runtime_error("timed out waiting for cloud-init to complete"); };
    mp::utils::try_action_for(on_timeout, timeout, action);
}

mp::Path mp::utils::make_dir(const QDir& a_dir, const QString& name)
{
    if (!a_dir.mkpath(name))
    {
        QString dir{a_dir.filePath(name)};
        throw std::runtime_error(fmt::format("unable to create directory '{}'", dir.toStdString()));
    }
    return a_dir.filePath(name);
}

QString mp::utils::make_uuid()
{
    auto uuid = QUuid::createUuid().toString();

    // Remove curly brackets enclosing uuid
    return uuid.mid(1, uuid.size() - 2);
}

std::string mp::utils::contents_of(const multipass::Path& file_path)
{
    const std::string name{file_path.toStdString()};
    std::ifstream in(name, std::ios::in | std::ios::binary);
    if (!in)
        throw std::runtime_error(fmt::format("failed to open file '{}': {}({})", name, strerror(errno), errno));

    std::stringstream stream;
    stream << in.rdbuf();
    return stream.str();
}

bool mp::utils::has_only_digits(const std::string& value)
{
    return std::all_of(value.begin(), value.end(), [](char c) { return std::isdigit(c); });
}

void mp::utils::validate_server_address(const std::string& address)
{
    if (address.empty())
        throw std::runtime_error("empty server address");

    const auto tokens = mp::utils::split(address, ":");
    const auto server_name = tokens[0];
    if (tokens.size() == 1u)
    {
        if (server_name == "unix")
            throw std::runtime_error(fmt::format("missing socket file in address '{}'", address));
        else
            throw std::runtime_error(fmt::format("missing port number in address '{}'", address));
    }

    const auto port = tokens[1];
    if (server_name != "unix" && !mp::utils::has_only_digits(port))
        throw std::runtime_error(fmt::format("invalid port number in address '{}'", address));
}
