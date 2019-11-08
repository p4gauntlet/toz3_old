#include "codegen.h"

namespace TOZ3 {

Visitor::profile_t CodeGenToz3::init_apply(const IR::Node* node) {
    return Inspector::init_apply(node);
}

void CodeGenToz3::end_apply(const IR::Node *) {
    if (nullptr != outStream) {
        cstring res = builder->toString();
        *outStream << res.c_str();
        outStream->flush();
    }
}

bool CodeGenToz3::preorder(const IR::P4Program* p) {
    builder->append("from p4z3.expressions import *\n\n");
    builder->newline();
    builder->newline();
    builder->append("def p4_program(z3_reg):");
    builder->newline();
    builder->append(depth, "###### HARDCODED ######");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"error\", z3.BitVecSort(8))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"T\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"O\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"D\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"M\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"HashAlgorithm\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"CloneType\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"packet_in\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "z3_reg.register_typedef(\"packet_out\", z3.BitVecSort(1))");
    builder->newline();
    builder->append(depth, "###### END HARDCODED ######");
    builder->newline();
    builder->newline();
    for (auto o: p->objects)
        visit(o);
    builder->appendFormat(depth, "return z3_reg, main");
    return false;
}

bool CodeGenToz3::preorder(const IR::P4Parser* p) {


    auto parser_name = p->name.name;
    builder->appendFormat(depth, "###### PARSER %s ######", parser_name);
    builder->newline();
    // output header
    builder->appendFormat(depth, "%s_args = [\n", parser_name);
    // for (auto cp : p->getApplyParameters()->parameters) {
    //     builder->append(depth+1,"(");
    //     visit(cp);
    //     builder->append("),");
    //     builder->newline();
    // }
    builder->append(depth, "]\n");
    builder->appendFormat(depth, "%s = P4Parser()", parser_name);
    builder->newline();
    builder->appendFormat(depth, "%s.add_args(%s_args)", parser_name, parser_name);
    builder->newline();
    builder->newline();

    return false;
}

bool CodeGenToz3::preorder(const IR::P4Control* c) {

    auto ctrl_name = c->name.name;
    builder->appendFormat(depth, "###### CONTROL %s ######", ctrl_name);
    builder->newline();
    // output header
    builder->appendFormat(depth, "%s_args = [\n", ctrl_name);
    for (auto cp : c->getApplyParameters()->parameters) {
        builder->append(depth+1,"(");
        visit(cp);
        builder->append("),");
        builder->newline();
    }
    builder->append(depth, "]\n");
    builder->appendFormat(depth, "%s = P4Control()", ctrl_name);
    builder->newline();
    builder->appendFormat(depth, "%s.add_instance(z3_reg, \"inouts\", %s_args)", ctrl_name, ctrl_name);
    builder->newline();
    builder->newline();

    /*
     * (1) Action
     * (2) Tables
     * (3) Instance Declarations
     */
    for (auto a : c->controlLocals) {
            visit(a);
            if (a->is<IR::Declaration_Variable>())
                builder->appendFormat(depth, "%s.add_apply_stmt(stmt)", ctrl_name);
            else
             builder->appendFormat(depth, "%s.declare_local(\"%s\", %s)", ctrl_name, a->name.name, a->name.name);
         builder->newline();
         builder->newline();
    }

    /*
     * (3) Apply Part
     */
    builder->appendFormat(depth, "###### CONTROL %s APPLY ######", ctrl_name);
    builder->newline();
    visit(c->body);
    builder->appendFormat(depth, "%s.add_apply_stmt(stmt)", ctrl_name);
    builder->newline();
    builder->newline();
    return false;
}


bool CodeGenToz3::preorder(const IR::Type_Extern* t) {
    auto extern_name = t->name.name;
    builder->appendFormat(depth, "###### EXTERN %s ######", extern_name);
    builder->newline();
    builder->appendFormat(depth, "%s = P4Extern(\"%s\", z3_reg)", extern_name, extern_name);
    builder->newline();
    for (auto param : t->typeParameters->parameters) {
        builder->appendFormat(depth, "%s.add_parameter(", extern_name);
        visit(param);
        builder->append(")");
        builder->newline();
    }
    builder->appendFormat(depth, "z3_reg.register_extern(\"%s\", %s)",
             extern_name, extern_name);
    builder->newline();
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::Method* t) {
    auto method_name = t->name.name;
    // skip assert, this causes python to break ha.
    // TODO: FIX
    if (method_name == "assert")
        return false;

    builder->appendFormat(depth, "###### METHOD %s ######", method_name);
    builder->newline();
    builder->appendFormat(depth, "%s = P4Extern(\"%s\", z3_reg)", method_name, method_name);
    builder->newline();
    for (auto param : t->getParameters()->parameters) {
        builder->appendFormat(depth, "%s.add_parameter(", method_name);
        visit(param);
        builder->append(")");
        builder->newline();
    }
    builder->appendFormat(depth, "z3_reg.register_extern(\"%s\", %s)",
             method_name, method_name);
    builder->newline();
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::P4Action* p4action) {
    auto action_name = p4action->name.name;
    builder->appendFormat(depth, "###### ACTION %s ######", action_name);
    builder->newline();
    builder->appendFormat(depth, "%s = P4Action()", action_name);
    builder->newline();

    for (auto param : p4action->parameters->parameters) {
        builder->appendFormat(depth, "%s.add_parameter(", action_name);
        visit(param);
        builder->append(")");
        builder->newline();
    }

    // body BlockStatement
    visit(p4action->body);

    builder->appendFormat(depth, "%s.add_stmt(stmt)", action_name);
    builder->newline();
    builder->appendFormat(depth, "z3_reg.register_extern(\"%s\", %s)",
             action_name, action_name);
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::P4Table* p4table) {
    tab_name = p4table->name.name;
    builder->appendFormat(depth, "###### TABLE %s ######", tab_name);
    builder->newline();
    builder->appendFormat(depth, "%s = P4Table(\"%s\")", tab_name, tab_name);
    builder->newline();
    for (auto p : p4table->properties->properties) {
        // IR::Property
        visit(p);
    }
    return false;
}

bool CodeGenToz3::preorder(const IR::Property* p) {
    // Tao: a trick here
    if (p->name.name == "default_action") {
        builder->appendFormat(depth, "%s.add_default(", tab_name);
        visit(p->value);
        builder->append(")");
        builder->newline();
    }
    else if (p->name.name == "size") {
        // skip it
    }
    else {
        visit(p->value);
    }
    return false;
}

bool CodeGenToz3::preorder(const IR::ActionList* acl) {
    // Tao: a trick here
    for (auto act: acl->actionList) {
        for (const auto* anno : act->getAnnotations()->annotations) {
            if (anno->name.name == "defaultonly")
                continue;
        }
        builder->appendFormat(depth, "%s.add_action(", tab_name);
        visit(act->expression);
        builder->append(")");
        builder->newline();
    }
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::Key* key) {
    for (auto ke : key->keyElements) {
        builder->append(depth, "table_key = ");
        visit(ke->expression);
        builder->newline();
        builder->appendFormat(depth, "%s.add_match(table_key)",
                tab_name);
        builder->newline();
    }
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::KeyElement* ke) {
    visit(ke->expression);
    return false;
}

bool CodeGenToz3::preorder(const IR::ExpressionValue* ev) {
    visit(ev->expression);
    return false;
}

bool CodeGenToz3::preorder(const IR::MethodCallExpression* mce) {
    if (!is_inswitchstmt) {
        builder->append("MethodCallExpr(");
        visit(mce->method);
        if (mce->arguments->size() != 0) {
            // output arguments
            for (size_t i=0; i<mce->arguments->size(); i++) {
                builder->append(", ");
                visit(mce->arguments->at(i)->expression);
            }
        }
        builder->append(")");
    }
    else {
        key_words.insert("apply");
        visit(mce->method);
        key_words.erase("apply");
    }

    return false;
}

bool CodeGenToz3::preorder(const IR::ListExpression* le) {
    bool first = true;
    builder->append("[");
    for (auto e : le->components) {
        if (!first)
            builder->append(", ");
        first = false;
        visit(e);
    }
    builder->append("]");
    return false;
}

bool CodeGenToz3::preorder(const IR::BlockStatement* b) {
    // top part
    builder->append(depth, "def BLOCK():\n");
    builder->append(depth+1, "block = BlockStatement()\n");
    // body part
    depth++;
    for (auto c : b->components) {
        visit(c);
        builder->append(depth, "block.add(stmt)\n");
    }
    depth--;

    // bot part
    builder->newline();
    builder->append(depth+1, "return block\n\n");
    builder->append(depth, "stmt = BLOCK()\n\n");

    return false;
}

bool CodeGenToz3::preorder(const IR::AssignmentStatement* as) {
    builder->append(depth, "lval = ");
    // Tao: slice assignment
    if (as->left->is<IR::Slice>())
        visit(as->left->to<IR::Slice>()->e0);
    else
        visit(as->left);
    builder->newline();
    builder->append(depth, "rval = ");
    visit(as->right);
    builder->newline();

    // Tao: slice assignment
    if (auto sl = as->left->to<IR::Slice>()) {
        builder->append(depth,"stmt = SliceAssignment(lval, rval, ");
        builder->appendFormat("%d, %d)", sl->getH(), sl->getL());
        builder->newline();
    }
    else {
        builder->append(depth, "stmt = AssignmentStatement(lval, rval)");
        builder->newline();
    }
    return false;
}

bool CodeGenToz3::preorder(const IR::MethodCallStatement* mcs) {
    builder->append(depth, "stmt = MethodCallStmt(");
    visit(mcs->methodCall);
    builder->append(")");
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::IfStatement* ifs) {
    builder->append(depth, "if_block = IfStatement()\n\n");
    // basically, ifs->condition is an expression
    builder->append(depth, "condition = ");
    visit(ifs->condition);
    builder->newline();
    builder->append(depth, "if_block.add_condition(condition)\n\n\n");

    visit(ifs->ifTrue);
    builder->append(depth, "if_block.add_then_stmt(stmt)\n\n\n");

    if (ifs->ifFalse != nullptr) {
        visit(ifs->ifFalse);
        builder->append(depth, "if_block.add_else_stmt(stmt)\n\n\n");
    }

    builder->append(depth, "stmt = if_block\n");

    return false;
}

bool CodeGenToz3::preorder(const IR::Neg* expr) {
    builder->append("P4neg(");
    visit(expr->expr);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Cmpl* expr) {
    builder->append("P4inv(");
    visit(expr->expr);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::LNot* expr) {
    builder->append("P4not(");
    visit(expr->expr);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Mul* expr) {
    builder->append("P4mul(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Div* expr) {
    builder->append("P4div(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Mod* expr) {
    builder->append("P4mod(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Add* expr) {
    builder->append("P4add(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Sub* expr) {
    builder->append("P4sub(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Shl* expr) {
    builder->append("P4lshift(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Shr* expr) {
    builder->append("P4rshift(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Equ* expr) {
    // a single line
    builder->append("P4eq(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Neq* expr) {
    // a single line
    builder->append("P4ne(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")\n");
    return false;
}

bool CodeGenToz3::preorder(const IR::Lss* expr) {
    builder->append("P4lt(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Leq* expr) {
    builder->append("P4le(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Grt* expr) {
    builder->append("P4gt(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Geq* expr) {
    builder->append("P4ge(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::BAnd* expr) {
    builder->append("P4band(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::BOr* expr) {
    builder->append("P4bor(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::BXor* expr) {
    builder->append("P4xor(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::LAnd* expr) {
    builder->append("P4land(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::LOr* expr) {
    builder->append("P4lor(");
    visit(expr->left);
    builder->append(", ");
    visit(expr->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Member* m) {

    visit(m->expr);

    if (key_words.find(m->member.name) != key_words.end()) {
        // if the name of the expression is a special keyword, skip it
        return false;

    }
    builder->appendFormat("\".%s\"",  m->member.name);

    return false;
}

bool CodeGenToz3::preorder(const IR::DefaultExpression*) {
    builder->appendFormat("\"default\"");
    return false;
}


bool CodeGenToz3::preorder(const IR::PathExpression* p) {

    if (key_words.find(p->path->asString()) != key_words.end()) {
        // if the name of the expression is a special keyword, skip it
        return false;
    }
    builder->appendFormat("\"%s\"", p->path->asString());
    return false;
}

bool CodeGenToz3::preorder(const IR::Constant* c) {

    // Ideally we would like to define a z3 var here and then interpret it
    // so we just need to call visit()
    // Unfortunately, z3 python does not handle Var declarations nicely
    // So for now we need to do hardcoded checks.
    // builder->appendFormat("z3.Var(%d, ", c->value.get_ui());
    // visit(c->type);
    // builder->append(")");
    if (auto tb = c->type->to<IR::Type_Bits>())
        builder->appendFormat("z3.BitVecVal(%s, %d)",
            c->toString(), tb->size);
    else if (c->type->is<IR::Type_InfInt>())
        builder->appendFormat("%d", c->value.get_ui());
    else
        FATAL_ERROR("Constant Node %s not implemented!",
                    c->type->node_type_name());
    return false;
}

bool CodeGenToz3::preorder(const IR::BoolLiteral* bl) {
    if (bl->value == true)
        builder->append("z3.BoolVal(True)");
    else
        builder->append("z3.BoolVal(False)");
    return false;
}

bool CodeGenToz3::preorder(const IR::Cast* c) {
    builder->append("P4Cast(");
    visit(c->expr);
    builder->append(", ");
    visit(c->destType);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Concat* c) {
    builder->append("P4Concat(");
    visit(c->left);
    builder->append(", ");
    visit(c->right);
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Slice* s) {
    builder->append("P4Slice(");
    visit(s->e0);
    builder->append(", ");
    visit(s->e1);
    // Tao: assume it always be integer constant
    builder->append(", ");
    visit(s->e2);
    // Tao: assume it always be integer constant
    builder->append(")");
    return false;
}

bool CodeGenToz3::preorder(const IR::Parameter* p) {
    /*
     * p->
     * (1) direction, inout, in, out
     * (2) type
     * (3) id for name
     */
    if (p->direction == IR::Direction::Out ||
        p->direction == IR::Direction::InOut)
        builder->append("True, ");
    else
        builder->append("False, ");

    builder->append("\"");
    builder->append(p->name);
    builder->append("\", ");
    visit(p->type);

    return false;
}

bool CodeGenToz3::preorder(const IR::Declaration_Variable* dv) {
    builder->append(depth, "lval = \"");
    builder->append(dv->name.name);
    builder->append("\"\n");
    builder->append(depth, "rval = ");
    if (nullptr != dv->initializer)
        visit(dv->initializer);
    else {
        builder->append("z3_reg.instance(\"");
        builder->append(dv->name.name);
        builder->append("\", ");
        visit(dv->type);
        builder->append(")");
    }
    builder->newline();
    builder->append(depth, "stmt = P4Declaration(lval, rval)");
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::SwitchStatement* ss) {
    builder->append(depth, "switch_block = SwitchStatement(");

    is_inswitchstmt = true;
    visit(ss->expression);
    is_inswitchstmt = false;
    builder->append(")\n");

    // switch case
    for (size_t i=0; i<ss->cases.size(); i++)
        visit(ss->cases.at(i));
    builder->append(depth, "stmt = switch_block\n\n");


    return false;
}

bool CodeGenToz3::preorder(const IR::SwitchCase* sc) {
    int if_output_def_blk = 0;

    builder->append(depth, "switch_block.add_case(");
    visit(sc->label);
    builder->append(")\n");

    if_output_def_blk = sc->statement->is<IR::BlockStatement>()?0:1;
    if (if_output_def_blk) {
        builder->append(depth, "def BLOCK():\n");
        builder->append(depth+1, "block = BlockStatement()\n");
        depth++;
    }
    visit(sc->statement);

    if (if_output_def_blk) {
        depth--;
        builder->append(depth+1, "return block\n");
        builder->append(depth, "stmt = BLOCK()\n\n");
    }

    builder->append(depth, "switch_block.add_stmt_to_case(");
    visit(sc->label);
    builder->append(", stmt)\n");
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Header* t) {

    builder->appendFormat("%sz3_args = [\n", builder->indent(depth));
    for (auto f : t->fields) {
        builder->appendFormat("%s(\'%s\', ", builder->indent(depth+1), f->name.name);
        visit(f->type);
        builder->append("),\n");
    }
    builder->appendFormat("%s]\n", builder->indent(depth));
    builder->appendFormat("%sz3_reg.register_header(\"%s\", z3_args)\n\n", builder->indent(depth), t->name.name);

    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Package* t) {
    builder->appendFormat(depth, "class %s():", t->getName().name);
    builder->newline();
    builder->append(depth+1, "def __init__(self, ");
    for (auto cp : t->getConstructorParameters()->parameters)
        builder->appendFormat("%s, ", cp->name.name);
    builder->append("):");
    builder->newline();
    for (auto cp : t->getConstructorParameters()->parameters) {
        builder->appendFormat(depth+2, "self.%s = %s",
         cp->name.name, cp->name.name);
        builder->newline();
    }
    builder->newline();
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Struct* t) {

    builder->appendFormat("%sz3_args = [\n", builder->indent(depth));
    for (auto f : t->fields) {
        builder->appendFormat("%s(\'%s\', ", builder->indent(depth+1), f->name.name);
        visit(f->type);
        builder->append("),\n");
    }
    builder->appendFormat("%s]\n", builder->indent(depth));
    builder->appendFormat("%sz3_reg.register_struct(\"%s\", z3_args)\n\n", builder->indent(depth), t->name.name);

    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Typedef* t) {
    builder->appendFormat("%sz3_reg.register_typedef(\"%s\", ", builder->indent(depth), t->name.name);
    visit(t->type);
    builder->appendFormat(")\n");
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Bits* t) {
    builder->appendFormat("z3.BitVecSort(%d)", t->size);
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Boolean*) {
    builder->appendFormat("z3.BoolSort()");
    return false;
}


bool CodeGenToz3::preorder(const IR::Type_Varbits* t) {\
    ::warning("Replacing Varbit  %1% with Bitvector.",t);
    builder->appendFormat("z3.BitVecSort(%d)", t->size);
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Name* t) {
    builder->appendFormat("z3_reg.type(\"%s\")", t->path->name.name);
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_Stack* type) {
    builder->append("z3_reg.stack(");
    visit(type->elementType);
    builder->appendFormat(", %d)", type->getSize());
    return false;
}

bool CodeGenToz3::preorder(const IR::Type_InfInt*) {
    builder->append("z3.IntSort()");
    return false;
}

bool CodeGenToz3::preorder(const IR::ArrayIndex* a) {
    visit(a->left);
    builder->append("\".");
    visit(a->right);
    builder->append("\"");
    return false;
}

// for V1Switch Declaration Instance
bool CodeGenToz3::preorder(const IR::Declaration_Instance* di) {
    builder->appendFormat(depth, "%s = ", di->name.name);

    if (auto tp = di->type->to<IR::Type_Specialized>()) {
        builder->append(tp->baseType->toString());
    } else if (auto tn = di->type->to<IR::Type_Name>()) {
        builder->append(tn->path->name.name);
    }


    if (di->arguments->size() > 0 )
        builder->append("(");
    for (size_t i=0; i<di->arguments->size(); i++) {
        const IR::Argument* const arg = di->arguments->at(i);
        // std::cout << arg->name.name << std::endl;
        if (auto cce = arg->expression->to<IR::ConstructorCallExpression>()) {
            if (arg->name.name != nullptr)
                builder->appendFormat("%s=", arg->name.name);
            builder->appendFormat("%s, ",
              cce->toString(), cce->toString());
        } else {
            FATAL_ERROR("Declaration Node %s not implemented!",
                        arg->expression->node_type_name());
        }
    }
    if (di->arguments->size() > 0 )
        builder->append(")");
    builder->newline();

    return false;
}


} // namespace TOZ3
