// Copyright 2015-2020 Tier IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OPENSCENARIO_INTERPRETER__SCOPE_HPP_
#define OPENSCENARIO_INTERPRETER__SCOPE_HPP_

#include <boost/filesystem.hpp>
#include <memory>
#include <openscenario_interpreter/syntax/entity_ref.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openscenario_interpreter
{
class EnvironmentFrame
{
  friend struct Scope;

  const std::string scope_name;

  std::unordered_multimap<std::string, Element> environments;

  EnvironmentFrame * const parent = nullptr;

  std::unordered_multimap<std::string, EnvironmentFrame *> named_children;

  std::vector<EnvironmentFrame *> anonymous_children;

  explicit EnvironmentFrame() = default;

  explicit EnvironmentFrame(EnvironmentFrame &, const std::string &);

  explicit EnvironmentFrame(const EnvironmentFrame &) = delete;

  explicit EnvironmentFrame(EnvironmentFrame &&) = delete;

public:
  auto findElement(const std::string &) const -> Element;

  auto getQualifiedName() const -> std::string;

  auto insert(const std::string &, Element) -> void;

private:
  /*  */ auto lookupChildElement(const std::string &) const -> Element;

  /*  */ auto lookupChildScope(const std::string &) const -> std::list<const EnvironmentFrame *>;

  static auto lookupQualifiedElement(
    const EnvironmentFrame *, std::vector<std::string>::iterator,
    std::vector<std::string>::iterator) -> Element;

  /*  */ auto lookupUnqualifiedElement(const std::string &) const -> Element;

  /*  */ auto lookupUnqualifiedScope(const std::string &) const -> const EnvironmentFrame *;
};

class Scope
{
  const std::shared_ptr<EnvironmentFrame> frame;

  struct GlobalEnvironment
  {
    const boost::filesystem::path pathname;  // for substitution syntax '$(dirname)'

    std::unordered_map<std::string, Element> entities;  // ScenarioObject or EntitySelection

    explicit GlobalEnvironment(const boost::filesystem::path &);

    auto entityRef(const EntityRef &) const -> Element;  // TODO: RETURN ScenarioObject TYPE!

    auto isAddedEntity(const EntityRef &) const -> bool;
  };

  const std::shared_ptr<GlobalEnvironment> global_environment;

public:
  const std::string name;

  std::list<EntityRef> actors;

  explicit Scope() = delete;

  explicit Scope(const boost::filesystem::path &);

private:
  explicit Scope(const Scope &, const std::string &, const std::shared_ptr<EnvironmentFrame> &);

public:
  Scope(const Scope &) = default;  // NOTE: shallow copy

  Scope(Scope &&) noexcept = default;

  auto findElement(const std::string & name_) const -> Element;

  auto global() const -> const GlobalEnvironment &;

  auto global() -> GlobalEnvironment &;

  auto localScope() const noexcept -> const Scope &;

  auto localScope() noexcept -> Scope &;

  auto makeChildScope(const std::string &) const -> Scope;

  auto insert(const std::string & name_, const Element & element) -> void;
};
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__SCOPE_HPP_
