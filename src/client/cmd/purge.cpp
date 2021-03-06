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

#include "purge.h"

#include <multipass/cli/argparser.h>

namespace mp = multipass;
namespace cmd = multipass::cmd;
using RpcMethod = mp::Rpc::Stub;

mp::ReturnCode cmd::Purge::run(mp::ArgParser* parser)
{
    auto ret = parse_args(parser);
    if (ret != ParseCode::Ok)
    {
        return parser->returnCodeFrom(ret);
    }

    auto on_success = [](mp::PurgeReply& reply) { return mp::ReturnCode::Ok; };

    auto on_failure = [this](grpc::Status& status) {
        cerr << "purge failed: " << status.error_message() << "\n";
        return mp::ReturnCode::CommandFail;
    };

    mp::PurgeRequest request;
    return dispatch(&RpcMethod::purge, request, on_success, on_failure);
}

std::string cmd::Purge::name() const
{
    return "purge";
}

QString cmd::Purge::short_help() const
{
    return QStringLiteral("Purge all deleted instances permanently");
}

QString cmd::Purge::description() const
{
    return QStringLiteral("Purge all deleted instances permanently, including all their data.");
}

mp::ParseCode cmd::Purge::parse_args(mp::ArgParser* parser)
{
    auto status = parser->commandParse(this);

    if (status != ParseCode::Ok)
    {
        return status;
    }

    if (parser->positionalArguments().count() > 0)
    {
        cerr << "This command takes no arguments\n";
        return ParseCode::CommandLineError;
    }

    return status;
}
