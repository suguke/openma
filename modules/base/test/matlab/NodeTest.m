classdef NodeTest < matlab.unittest.TestCase
    methods (Test)
    
        function constructorWithParent(testCase)
            root = ma.Node('root');
            child = ma.Node('child', root);
            testCase.verifyEqual(root.hasChildren(), true);
            testCase.verifyEqual(child.hasParents(), true);
            testCase.verifyEqual(root.name(), 'root');
            testCase.verifyEqual(child.name(), 'child');
            delete(root);
            testCase.verifyEqual(child.hasParents(), false);
        end

        function child0(testCase)
            root = ma.Node('root');
            testCase.verifyError(@()root.child(0), 'SWIG:Node:child');
        end

        function child1(testCase)
            root = ma.Node('root');
            ma.Node('child', root);
            child = root.child(1);
            testCase.verifyEqual(child.name(), 'child');
            testCase.verifyEqual(child.hasParents(), true);
            delete(root);
            testCase.verifyEqual(child.name(), 'child');
            testCase.verifyEqual(child.hasParents(), false);
        end

        function children(testCase)
            root = ma.Node('root');
            child1 = ma.Node('child1', root);
            child2 = ma.Node('child2', root);
            children = root.children();
            testCase.verifyEqual(length(children), 2);
            testCase.verifyEqual(children{1}.name(), child1.name());
            testCase.verifyEqual(children{2}.name(), child2.name());
            delete(root);
            testCase.verifyEqual(children{1}.hasParents(), false);
            testCase.verifyEqual(children{2}.hasParents(), false);
            delete(child1);
            delete(child2);
            % Still exist?
            testCase.verifyEqual(children{1}.name(), 'child1');
            testCase.verifyEqual(children{2}.name(), 'child2');
            testCase.verifyEqual(children{1}.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(children{2}.property('_SWIG_ref_count').cast, 1);
        end

        function clearNode(testCase)
            root = ma.Node('root');
            child1 = ma.Node('child1', root);
            child2 = ma.Node('child2', root);
            testCase.verifyEqual(root.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(child1.property('_SWIG_ref_count').cast, 2);
            testCase.verifyEqual(child2.property('_SWIG_ref_count').cast, 2);
            root.clear();
            testCase.verifyEqual(root.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(child1.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(child2.property('_SWIG_ref_count').cast, 1);
        end

        function copyNode(testCase)
            root = ma.Node('root');
            child1 = ma.Node('child1', root);
            child2 = ma.Node('child2', child1);
            other = ma.Node('other');
            sub1 = ma.Node('sub1', other);
            other.copy(root);
            testCase.verifyEqual(sub1.hasParents, false);
            testCase.verifyEqual(root.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(child1.property('_SWIG_ref_count').cast, 2);
            testCase.verifyEqual(child2.property('_SWIG_ref_count').cast, 2);
            testCase.verifyEqual(other.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(sub1.property('_SWIG_ref_count').cast, 1);
            other.child(1).setName('Goal');
            other.child(1).child(1).setName('Goal2');
            testCase.verifyEqual(child1.name(),'child1');
            testCase.verifyEqual(child2.name(),'child2');
        end

        function cloneNode(testCase)
            root = ma.Node('root');
            ma.Node('child1', root);
            other = root.clone();
            other.setName('Foo');
            other.child(1).setName('Bar');
            testCase.verifyEqual(root.hasChildren, true);
            testCase.verifyEqual(other.hasChildren, true);
            testCase.verifyEqual(root.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(other.property('_SWIG_ref_count').cast, 1);
            testCase.verifyEqual(root.name(),'root');
            testCase.verifyEqual(other.name(),'Foo');
            testCase.verifyEqual(root.child(1).name(),'child1');
            testCase.verifyEqual(other.child(1).name(),'Bar');
        end
    end
end